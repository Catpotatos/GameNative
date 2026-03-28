package com.winlator.renderer;

import android.opengl.GLES11Ext;
import android.opengl.GLES20;

// import com.winlator.XrActivity;
import com.winlator.xserver.Drawable;

import java.nio.ByteBuffer;

public class Texture {
    protected int textureId = 0;
    private int wrapS = GLES20.GL_CLAMP_TO_EDGE;
    private int wrapT = GLES20.GL_CLAMP_TO_EDGE;
    private int magFilter = GLES20.GL_LINEAR;
    private int minFilter = GLES20.GL_LINEAR;
    private int format = GLES11Ext.GL_BGRA;
    protected byte unpackAlignment = 4;
    protected boolean needsUpdate = true;

    protected void generateTextureId() {
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        this.textureId = textureIds[0];
    }

    protected void setTextureParameters() {
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, this.wrapS);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, this.wrapT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, this.magFilter);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, this.minFilter);
    }

    public void allocateTexture(short width, short height, ByteBuffer data) {
        generateTextureId();

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glPixelStorei(GLES20.GL_UNPACK_ALIGNMENT, unpackAlignment);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);

        if (data != null) {
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, format, width, height, 0, format, GLES20.GL_UNSIGNED_BYTE, data);
        }
        setTextureParameters();
        GLES20.glBindTexture(3553, 0);
    }

    public void setNeedsUpdate(boolean needsUpdate) {
        this.needsUpdate = needsUpdate;
    }

    private static long lastNullDataLogTime = 0;
    private static int nullDataCount = 0;

    public void updateFromDrawable(Drawable drawable) {
        ByteBuffer data = drawable.getData();
        if (data == null) {
            // Throttled warning: indicates GPUImage replaced the data buffer with null
            nullDataCount++;
            long now = System.currentTimeMillis();
            if (now - lastNullDataLogTime > 2000) {
                android.util.Log.w("Texture", "updateFromDrawable: data is NULL for drawable "
                    + drawable.width + "x" + drawable.height + " (id=" + drawable.id
                    + "), dropped " + nullDataCount + " updates");
                nullDataCount = 0;
                lastNullDataLogTime = now;
            }
            return;
        }

        if (!isAllocated()) {
            allocateTexture(drawable.width, drawable.height, data);
        }
        else {
            // Always upload texture data from the drawable's buffer.
            // This ensures frames are never missed regardless of whether
            // forceUpdate/needsUpdate was triggered (e.g. Wine's x11drv
            // window surface flush via XShmPutImage or direct SHM writes).
            // GPUImage overrides this method and skips the upload since
            // its hardware-buffer-backed EGLImage auto-reflects changes.
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
            GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, drawable.width, drawable.height, format, GLES20.GL_UNSIGNED_BYTE, data);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
            needsUpdate = false;
        }
    }

    public boolean isAllocated() {
        return textureId > 0;
    }

    public int getTextureId() {
        return textureId;
    }

    public void copyFromFramebuffer(int framebuffer, short width, short height) {
        if (!isAllocated()) {
            allocateTexture(width, height, null);
        }
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, framebuffer);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
        GLES20.glCopyTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, 0, 0, width, height, 0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
    }

    public void invalidate() {
        textureId = 0;
        needsUpdate = true;
    }

    public void destroy() {
        if (textureId > 0) {
            int[] textureIds = new int[]{textureId};
            GLES20.glDeleteTextures(textureIds.length, textureIds, 0);
            textureId = 0;
        }
    }
}
