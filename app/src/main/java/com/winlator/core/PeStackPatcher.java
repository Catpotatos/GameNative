    package com.winlator.core;

import android.util.Log;

import java.io.File;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Patches the SizeOfStackReserve / SizeOfStackCommit fields in a PE header
 * to ensure a minimum stack size.  This is needed because Wine reads the
 * PE header of wow64 guest processes to allocate the main-thread stack,
 * and the FEX/WOWBox64 translation layer adds significant per-call stack
 * overhead (2-4× per x86→ARM64 transition).  Many 32-bit games ship with
 * the default 1 MB SizeOfStackReserve, which is not enough under
 * translation.
 * <p>
 * PE format reference (relevant offsets):
 * <pre>
 *   DOS header
 *     +0x3C  e_lfanew        (int32)  → offset of PE signature
 *   PE signature              "PE\0\0"
 *   COFF header               20 bytes
 *   Optional header
 *     +0x00  Magic            (uint16)  0x10B = PE32, 0x20B = PE32+
 *     PE32:
 *       +0x48  SizeOfStackReserve  (uint32)
 *       +0x4C  SizeOfStackCommit   (uint32)
 *     PE32+:
 *       +0x48  SizeOfStackReserve  (uint64)
 *       +0x50  SizeOfStackCommit   (uint64)
 * </pre>
 */
public final class PeStackPatcher {

    private static final String TAG = "PeStackPatcher";

    /** Default minimum stack reserve: 4 MB. */
    public static final long DEFAULT_MIN_STACK_RESERVE = 4L * 1024 * 1024;

    /** Default minimum stack commit: 1 MB. */
    public static final long DEFAULT_MIN_STACK_COMMIT = 1L * 1024 * 1024;

    private PeStackPatcher() { }

    /**
     * Ensures that the PE executable at {@code exeFile} has at least
     * {@code minStackReserve} bytes of SizeOfStackReserve and
     * {@code minStackCommit} bytes of SizeOfStackCommit.
     *
     * @return {@code true} if the file was patched (or already had sufficient
     *         values), {@code false} on error.
     */
    public static boolean ensureMinimumStackSize(File exeFile, long minStackReserve, long minStackCommit) {
        if (exeFile == null || !exeFile.exists() || !exeFile.isFile()) {
            Log.w(TAG, "File does not exist or is not a file: " + exeFile);
            return false;
        }

        try (RandomAccessFile raf = new RandomAccessFile(exeFile, "rw")) {
            // ── Read DOS header ──
            if (raf.length() < 0x40) {
                Log.w(TAG, "File too small for DOS header: " + exeFile.getName());
                return false;
            }

            // Check MZ magic
            byte[] mz = new byte[2];
            raf.readFully(mz);
            if (mz[0] != 'M' || mz[1] != 'Z') {
                Log.w(TAG, "Not a PE file (no MZ magic): " + exeFile.getName());
                return false;
            }

            // e_lfanew at offset 0x3C
            raf.seek(0x3C);
            int eLfanew = readInt32LE(raf);
            if (eLfanew < 0 || eLfanew + 4 > raf.length()) {
                Log.w(TAG, "Invalid e_lfanew: " + eLfanew);
                return false;
            }

            // ── Read PE signature ──
            raf.seek(eLfanew);
            byte[] peSig = new byte[4];
            raf.readFully(peSig);
            if (peSig[0] != 'P' || peSig[1] != 'E' || peSig[2] != 0 || peSig[3] != 0) {
                Log.w(TAG, "Not a PE file (no PE\\0\\0 signature): " + exeFile.getName());
                return false;
            }

            // ── COFF header (20 bytes) ──
            // Skip COFF header to reach Optional header
            long optionalHeaderOffset = eLfanew + 4 + 20;

            // ── Optional header magic ──
            raf.seek(optionalHeaderOffset);
            int magic = readUInt16LE(raf);

            boolean isPE32 = (magic == 0x10B);
            boolean isPE32Plus = (magic == 0x20B);
            if (!isPE32 && !isPE32Plus) {
                Log.w(TAG, "Unknown PE optional header magic: 0x" + Integer.toHexString(magic));
                return false;
            }

            // ── SizeOfStackReserve / SizeOfStackCommit ──
            // Both PE32 and PE32+ have these at optional header + 0x48.
            // PE32: 4 bytes each.  PE32+: 8 bytes each.
            long stackReserveOffset = optionalHeaderOffset + 0x48;
            long stackCommitOffset;

            boolean patched = false;

            if (isPE32) {
                stackCommitOffset = stackReserveOffset + 4;

                // Read current values (unsigned 32-bit)
                raf.seek(stackReserveOffset);
                long currentReserve = readUInt32LE(raf);
                long currentCommit = readUInt32LE(raf);

                Log.i(TAG, exeFile.getName() + " (PE32): SizeOfStackReserve=" +
                        currentReserve + " (" + (currentReserve / 1024) + " KB), SizeOfStackCommit=" +
                        currentCommit + " (" + (currentCommit / 1024) + " KB)");

                if (currentReserve < minStackReserve) {
                    Log.i(TAG, "Patching SizeOfStackReserve: " + currentReserve + " → " + minStackReserve);
                    raf.seek(stackReserveOffset);
                    writeUInt32LE(raf, minStackReserve);
                    patched = true;
                }

                if (currentCommit < minStackCommit) {
                    Log.i(TAG, "Patching SizeOfStackCommit: " + currentCommit + " → " + minStackCommit);
                    raf.seek(stackCommitOffset);
                    writeUInt32LE(raf, minStackCommit);
                    patched = true;
                }
            } else {
                // PE32+
                stackCommitOffset = stackReserveOffset + 8;

                raf.seek(stackReserveOffset);
                long currentReserve = readInt64LE(raf);
                long currentCommit = readInt64LE(raf);

                Log.i(TAG, exeFile.getName() + " (PE32+): SizeOfStackReserve=" +
                        currentReserve + " (" + (currentReserve / 1024) + " KB), SizeOfStackCommit=" +
                        currentCommit + " (" + (currentCommit / 1024) + " KB)");

                if (currentReserve < minStackReserve) {
                    Log.i(TAG, "Patching SizeOfStackReserve: " + currentReserve + " → " + minStackReserve);
                    raf.seek(stackReserveOffset);
                    writeInt64LE(raf, minStackReserve);
                    patched = true;
                }

                if (currentCommit < minStackCommit) {
                    Log.i(TAG, "Patching SizeOfStackCommit: " + currentCommit + " → " + minStackCommit);
                    raf.seek(stackCommitOffset);
                    writeInt64LE(raf, minStackCommit);
                    patched = true;
                }
            }

            if (patched) {
                Log.i(TAG, "Successfully patched stack sizes in " + exeFile.getName());
            } else {
                Log.i(TAG, "No patching needed for " + exeFile.getName() +
                        " (stack already >= " + (minStackReserve / 1024 / 1024) + " MB)");
            }
            return true;

        } catch (Exception e) {
            Log.e(TAG, "Failed to patch PE header of " + exeFile.getName() + ": " + e.getMessage(), e);
            return false;
        }
    }

    /**
     * Convenience overload using default minimum sizes (4 MB reserve, 1 MB commit).
     */
    public static boolean ensureMinimumStackSize(File exeFile) {
        return ensureMinimumStackSize(exeFile, DEFAULT_MIN_STACK_RESERVE, DEFAULT_MIN_STACK_COMMIT);
    }

    // ── Little-endian I/O helpers ──

    private static int readInt32LE(RandomAccessFile raf) throws Exception {
        byte[] buf = new byte[4];
        raf.readFully(buf);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getInt();
    }

    private static long readUInt32LE(RandomAccessFile raf) throws Exception {
        return readInt32LE(raf) & 0xFFFFFFFFL;
    }

    private static int readUInt16LE(RandomAccessFile raf) throws Exception {
        byte[] buf = new byte[2];
        raf.readFully(buf);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getShort() & 0xFFFF;
    }

    private static long readInt64LE(RandomAccessFile raf) throws Exception {
        byte[] buf = new byte[8];
        raf.readFully(buf);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getLong();
    }

    private static void writeUInt32LE(RandomAccessFile raf, long value) throws Exception {
        byte[] buf = new byte[4];
        ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).putInt((int) (value & 0xFFFFFFFFL));
        raf.write(buf);
    }

    private static void writeInt64LE(RandomAccessFile raf, long value) throws Exception {
        byte[] buf = new byte[8];
        ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).putLong(value);
        raf.write(buf);
    }
}

