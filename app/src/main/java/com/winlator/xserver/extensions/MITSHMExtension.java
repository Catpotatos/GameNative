package com.winlator.xserver.extensions;

import static com.winlator.xserver.XClientRequestHandler.RESPONSE_CODE_SUCCESS;


import com.winlator.xconnector.XInputStream;
import com.winlator.xconnector.XOutputStream;
import com.winlator.xconnector.XStreamLock;
import com.winlator.xserver.Drawable;
import com.winlator.xserver.GraphicsContext;
import com.winlator.xserver.XClient;
import com.winlator.xserver.XLock;
import com.winlator.xserver.XServer;
import com.winlator.xserver.errors.BadDrawable;
import com.winlator.xserver.errors.BadGraphicsContext;
import com.winlator.xserver.errors.BadImplementation;
import com.winlator.xserver.errors.BadSHMSegment;
import com.winlator.xserver.errors.XRequestError;
import com.winlator.xserver.events.ShmCompletionEvent;

import java.io.IOException;
import java.nio.ByteBuffer;

public class MITSHMExtension implements Extension {
    public static final byte MAJOR_OPCODE = -101;

    // Throttled diagnostic counters for ShmPutImage
    private static long lastShmPutImageLogTime = 0;
    private static int shmPutImageCount = 0;

    private static abstract class ClientOpcodes {
        private static final byte QUERY_VERSION = 0;
        private static final byte ATTACH = 1;
        private static final byte DETACH = 2;
        private static final byte PUT_IMAGE = 3;
        private static final byte GET_IMAGE = 4;
    }

    @Override
    public String getName() {
        return "MIT-SHM";
    }

    @Override
    public byte getMajorOpcode() {
        return MAJOR_OPCODE;
    }

    @Override
    public byte getFirstErrorId() {
        return Byte.MIN_VALUE;
    }

    @Override
    public byte getFirstEventId() {
        return 64;
    }

    private static void queryVersion(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte)0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeShort((short)1);
            outputStream.writeShort((short)1);
            outputStream.writeShort((short)0);
            outputStream.writeShort((short)0);
            outputStream.writeByte((byte)0);
        }
    }

    private static void attach(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int xid = inputStream.readInt();
        int shmid = inputStream.readInt();
        inputStream.skip(4);
        client.xServer.getSHMSegmentManager().attach(xid, shmid);
    }

    private static void detach(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        client.xServer.getSHMSegmentManager().detach(inputStream.readInt());
    }

    private void putImage(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int drawableId = inputStream.readInt();
        int gcId = inputStream.readInt();
        short totalWidth = inputStream.readShort();
        short totalHeight = inputStream.readShort();
        short srcX = inputStream.readShort();
        short srcY = inputStream.readShort();
        short srcWidth = inputStream.readShort();
        short srcHeight = inputStream.readShort();
        short dstX = inputStream.readShort();
        short dstY = inputStream.readShort();
        byte depth = inputStream.readByte();
        byte format = inputStream.readByte();
        boolean sendEvent = inputStream.readByte() != 0;
        inputStream.skip(1); // padding
        int shmseg = inputStream.readInt();
        int offset = inputStream.readInt();

        Drawable drawable = client.xServer.drawableManager.getDrawable(drawableId);
        if (drawable == null) throw new BadDrawable(drawableId);

        GraphicsContext graphicsContext = client.xServer.graphicsContextManager.getGraphicsContext(gcId);
        if (graphicsContext == null) throw new BadGraphicsContext(gcId);

        ByteBuffer data = client.xServer.getSHMSegmentManager().getData(shmseg);
        if (data == null) throw new BadSHMSegment(shmseg);

        if (graphicsContext.getFunction() != GraphicsContext.Function.COPY) {
            throw new UnsupportedOperationException("GC Function other than COPY is not supported.");
        }

        // Apply the offset: position the buffer at the correct byte offset
        // within the shared memory segment where the image data starts.
        if (offset > 0 && offset < data.capacity()) {
            data = data.duplicate();
            data.position(offset);
            data = data.slice();
            data.order(java.nio.ByteOrder.LITTLE_ENDIAN);
        }

        drawable.drawImage(srcX, srcY, dstX, dstY, srcWidth, srcHeight, depth, data, totalWidth, totalHeight);

        // Send ShmCompletion event if requested — Wine's x11drv window surface
        // flush waits for this event before writing the next frame to the SHM buffer.
        // Without it, the client stalls and the display freezes.
        if (sendEvent) {
            byte eventCode = getFirstEventId(); // ShmCompletion event type
            client.sendEvent(new ShmCompletionEvent(
                eventCode, drawableId, MAJOR_OPCODE,
                ClientOpcodes.PUT_IMAGE, shmseg, offset));
        }

        // Throttled diagnostic: track ShmPutImage request rate
        shmPutImageCount++;
        long now = System.currentTimeMillis();
        if (now - lastShmPutImageLogTime > 2000) {
            if (shmPutImageCount > 0) {
                android.util.Log.d("MITSHMExtension", "ShmPutImage: " + shmPutImageCount + " requests in last 2s"
                    + " (drawableId=" + drawableId + ", " + srcWidth + "x" + srcHeight
                    + ", sendEvent=" + sendEvent + ", shmseg=" + shmseg + ")");
            }
            shmPutImageCount = 0;
            lastShmPutImageLogTime = now;
        }
    }

    @Override
    public void handleRequest(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int opcode = client.getRequestData();
        switch (opcode) {
            case ClientOpcodes.QUERY_VERSION :
                queryVersion(client, inputStream, outputStream);
                break;
            case ClientOpcodes.ATTACH :
                try (XLock lock = client.xServer.lock(XServer.Lockable.SHMSEGMENT_MANAGER)) {
                    attach(client, inputStream, outputStream);
                }
                break;
            case ClientOpcodes.DETACH :
                try (XLock lock = client.xServer.lock(XServer.Lockable.SHMSEGMENT_MANAGER)) {
                    detach(client, inputStream, outputStream);
                }
                break;
            case ClientOpcodes.PUT_IMAGE :
                try (XLock lock = client.xServer.lock(XServer.Lockable.SHMSEGMENT_MANAGER, XServer.Lockable.DRAWABLE_MANAGER, XServer.Lockable.GRAPHIC_CONTEXT_MANAGER)) {
                    putImage(client, inputStream, outputStream);
                }
                break;
            case ClientOpcodes.GET_IMAGE :
                // ShmGetImage — skip for now (not critical for rendering)
                client.skipRequest();
                break;
            default:
                throw new BadImplementation();
        }
    }
}
