package com.winlator.xserver.events;

import com.winlator.xconnector.XOutputStream;
import com.winlator.xconnector.XStreamLock;

import java.io.IOException;

/**
 * SHM Completion event — sent to the client after the X server finishes
 * reading shared memory data from an XShmPutImage request with send_event=True.
 *
 * Without this event, Wine's x11drv window surface flush stalls waiting for
 * confirmation that it is safe to modify the SHM buffer, causing a frozen image.
 *
 * Wire format (32 bytes):
 *   1  CARD8    type        (firstEventId of MIT-SHM extension)
 *   1  CARD8    unused
 *   2  CARD16   sequence number
 *   4  CARD32   drawable
 *   2  CARD16   minor event (SHM sub-opcode, 3 for PutImage)
 *   1  CARD8    major event (SHM major opcode)
 *   1           padding
 *   4  CARD32   shmseg
 *   4  CARD32   offset
 *  12           padding
 */
public class ShmCompletionEvent extends Event {
    private final int drawableId;
    private final byte majorOpcode;
    private final short minorOpcode;
    private final int shmseg;
    private final int offset;

    public ShmCompletionEvent(byte eventCode, int drawableId, byte majorOpcode, short minorOpcode, int shmseg, int offset) {
        super(eventCode & 0xFF);
        this.drawableId = drawableId;
        this.majorOpcode = majorOpcode;
        this.minorOpcode = minorOpcode;
        this.shmseg = shmseg;
        this.offset = offset;
    }

    @Override
    public void send(short sequenceNumber, XOutputStream outputStream) throws IOException {
        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(code);
            outputStream.writeByte((byte)0);
            outputStream.writeShort(sequenceNumber);
            outputStream.writeInt(drawableId);
            outputStream.writeShort(minorOpcode);
            outputStream.writeByte(majorOpcode);
            outputStream.writeByte((byte)0);   // padding
            outputStream.writeInt(shmseg);
            outputStream.writeInt(offset);
            outputStream.writePad(12);
        }
    }
}

