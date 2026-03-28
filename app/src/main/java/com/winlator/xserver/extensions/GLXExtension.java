package com.winlator.xserver.extensions;

import static com.winlator.xserver.XClientRequestHandler.RESPONSE_CODE_SUCCESS;

import android.util.Log;

import com.winlator.xconnector.XInputStream;
import com.winlator.xconnector.XOutputStream;
import com.winlator.xconnector.XStreamLock;
import com.winlator.xserver.XClient;
import com.winlator.xserver.XServer;
import com.winlator.xserver.errors.XRequestError;

import java.io.IOException;

/**
 * Minimal GLX extension stub for the X server.
 *
 * GL4ES handles all actual GLX functionality client-side (translating GLX calls
 * to EGL calls backed by ANGLE). This stub exists solely to make the X server
 * advertise "GLX" as a supported extension so that:
 *   1. Wine's winex11.drv proceeds with GL initialization (it checks for GLX via
 *      XQueryExtension before calling any glX* functions from libGL.so)
 *   2. GL4ES's glXQueryExtension() can succeed if it queries the X server
 *
 * The rendering path remains: Wine → GL4ES (GLX→EGL) → ANGLE (GLES→Vulkan) → GPU
 * This extension does NOT implement GLX rendering — that's GL4ES's job.
 *
 * Note: With LIBGL_FB=3 (pbuffer/FBO mode), GL4ES handles all GLX operations
 * client-side via EGL. These handlers are safety nets for the rare case where
 * GLX protocol messages reach the server (e.g., from Wine's direct X11 protocol
 * calls or from non-GL4ES GLX clients).
 */
public class GLXExtension implements Extension {
    private static final String TAG = "GLXExtension";
    public static final byte MAJOR_OPCODE = -105;

    // The XServer reference, used to look up the default visual ID dynamically
    private final XServer xServer;

    // GLX protocol sub-opcodes (X11 GLX specification)
    private static final int GLX_RENDER                     = 1;
    private static final int GLX_RENDER_LARGE               = 2;
    private static final int GLX_CREATE_CONTEXT             = 3;
    private static final int GLX_DESTROY_CONTEXT            = 4;
    private static final int GLX_MAKE_CURRENT               = 5;
    private static final int GLX_IS_DIRECT                  = 6;
    private static final int GLX_QUERY_VERSION              = 7;
    private static final int GLX_WAIT_GL                    = 8;
    private static final int GLX_WAIT_X                     = 9;
    private static final int GLX_COPY_CONTEXT               = 10;
    private static final int GLX_SWAP_BUFFERS               = 11;
    private static final int GLX_USE_X_FONT                 = 12;
    private static final int GLX_CREATE_GLX_PIXMAP          = 13;
    private static final int GLX_GET_VISUAL_CONFIGS         = 14;
    private static final int GLX_DESTROY_GLX_PIXMAP         = 15;
    private static final int GLX_VENDOR_PRIVATE             = 16;
    private static final int GLX_VENDOR_PRIVATE_WITH_REPLY  = 17;
    private static final int GLX_QUERY_EXTENSIONS_STRING    = 18;
    private static final int GLX_QUERY_SERVER_STRING        = 19;
    private static final int GLX_CLIENT_INFO                = 20;
    private static final int GLX_GET_FB_CONFIGS             = 21;
    private static final int GLX_CREATE_PIXMAP              = 22;
    private static final int GLX_DESTROY_PIXMAP             = 23;
    private static final int GLX_CREATE_NEW_CONTEXT         = 24;
    private static final int GLX_QUERY_CONTEXT              = 25;
    private static final int GLX_MAKE_CONTEXT_CURRENT       = 26;
    private static final int GLX_CREATE_PBUFFER             = 27;
    private static final int GLX_DESTROY_PBUFFER            = 28;
    private static final int GLX_GET_DRAWABLE_ATTRIBUTES    = 29;
    private static final int GLX_CHANGE_DRAWABLE_ATTRIBUTES = 30;
    private static final int GLX_CREATE_WINDOW              = 31;
    private static final int GLX_DESTROY_WINDOW             = 32;
    private static final int GLX_SET_CLIENT_INFO_ARB        = 33;
    private static final int GLX_CREATE_CONTEXT_ATTRIBS_ARB = 34;
    private static final int GLX_SET_CLIENT_INFO_2_ARB      = 35;

    // GLX version we advertise
    private static final int GLX_MAJOR_VERSION = 1;
    private static final int GLX_MINOR_VERSION = 4;

    // Auto-incrementing context tag for MakeCurrent responses
    private int nextContextTag = 1;

    public GLXExtension(XServer xServer) {
        this.xServer = xServer;
    }

    @Override
    public String getName() {
        return "GLX";
    }

    @Override
    public byte getMajorOpcode() {
        return MAJOR_OPCODE;
    }

    @Override
    public byte getFirstErrorId() {
        return 0;
    }

    @Override
    public byte getFirstEventId() {
        return 0;
    }

    @Override
    public void handleRequest(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException, XRequestError {
        int subOpcode = client.getRequestData() & 0xFF; // unsigned
        Log.d(TAG, "GLX request: subOpcode=" + subOpcode + " (" + subOpcodeName(subOpcode) + ")");

        switch (subOpcode) {
            // ── Queries that generate replies ──
            case GLX_QUERY_VERSION:
                handleQueryVersion(client, inputStream, outputStream);
                break;
            case GLX_GET_VISUAL_CONFIGS:
                handleGetVisualConfigs(client, inputStream, outputStream);
                break;
            case GLX_QUERY_EXTENSIONS_STRING:
                handleQueryExtensionsString(client, inputStream, outputStream);
                break;
            case GLX_QUERY_SERVER_STRING:
                handleQueryServerString(client, inputStream, outputStream);
                break;
            case GLX_GET_FB_CONFIGS:
                handleGetFBConfigs(client, inputStream, outputStream);
                break;
            case GLX_IS_DIRECT:
                handleIsDirect(client, inputStream, outputStream);
                break;
            case GLX_MAKE_CURRENT:
                handleMakeCurrent(client, inputStream, outputStream);
                break;
            case GLX_MAKE_CONTEXT_CURRENT:
                handleMakeContextCurrent(client, inputStream, outputStream);
                break;
            case GLX_QUERY_CONTEXT:
                handleQueryContext(client, inputStream, outputStream);
                break;
            case GLX_GET_DRAWABLE_ATTRIBUTES:
                handleGetDrawableAttributes(client, inputStream, outputStream);
                break;
            case GLX_VENDOR_PRIVATE_WITH_REPLY:
                handleVendorPrivateWithReply(client, inputStream, outputStream);
                break;

            // ── No-reply requests (consume input and acknowledge) ──
            case GLX_CREATE_CONTEXT:
                handleCreateContext(client, inputStream, outputStream);
                break;
            case GLX_CREATE_NEW_CONTEXT:
                handleCreateNewContext(client, inputStream, outputStream);
                break;
            case GLX_CREATE_CONTEXT_ATTRIBS_ARB:
                handleCreateContextAttribsARB(client, inputStream, outputStream);
                break;
            case GLX_DESTROY_CONTEXT:
                handleDestroyContext(client, inputStream, outputStream);
                break;
            case GLX_SWAP_BUFFERS:
                handleSwapBuffers(client, inputStream, outputStream);
                break;
            case GLX_CREATE_GLX_PIXMAP:
            case GLX_DESTROY_GLX_PIXMAP:
            case GLX_CREATE_PIXMAP:
            case GLX_DESTROY_PIXMAP:
            case GLX_CREATE_PBUFFER:
            case GLX_DESTROY_PBUFFER:
            case GLX_CREATE_WINDOW:
            case GLX_DESTROY_WINDOW:
            case GLX_CLIENT_INFO:
            case GLX_SET_CLIENT_INFO_ARB:
            case GLX_SET_CLIENT_INFO_2_ARB:
            case GLX_WAIT_GL:
            case GLX_WAIT_X:
            case GLX_COPY_CONTEXT:
            case GLX_USE_X_FONT:
            case GLX_RENDER:
            case GLX_RENDER_LARGE:
            case GLX_VENDOR_PRIVATE:
            case GLX_CHANGE_DRAWABLE_ATTRIBUTES:
                Log.d(TAG, "Skipping no-reply GLX sub-opcode: " + subOpcode
                    + " (" + subOpcodeName(subOpcode) + ")");
                client.skipRequest();
                break;

            default:
                Log.w(TAG, "Unhandled GLX sub-opcode: " + subOpcode + " — skipping");
                client.skipRequest();
                break;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Reply-generating handlers
    // ═══════════════════════════════════════════════════════════════════════

    private void handleQueryVersion(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int clientMajor = inputStream.readInt();
        int clientMinor = inputStream.readInt();
        Log.i(TAG, "QueryVersion: client=" + clientMajor + "." + clientMinor
            + ", server=" + GLX_MAJOR_VERSION + "." + GLX_MINOR_VERSION);

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeInt(GLX_MAJOR_VERSION);
            outputStream.writeInt(GLX_MINOR_VERSION);
            outputStream.writePad(16);
        }
    }

    private void handleGetVisualConfigs(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int screen = inputStream.readInt();
        Log.d(TAG, "GetVisualConfigs: screen=" + screen);

        int defaultVisualId = getDefaultVisualId();
        int numVisuals = 1;
        int numProps = 28;
        int[] config = buildVisualConfig(defaultVisualId);
        int replyLength = numVisuals * numProps;

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(replyLength);
            outputStream.writeInt(numVisuals);
            outputStream.writeInt(numProps);  // required by GLX protocol — was missing (treated as 0)
            outputStream.writePad(16);
            for (int prop : config) {
                outputStream.writeInt(prop);
            }
        }
    }

    private void handleGetFBConfigs(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int screen = inputStream.readInt();
        Log.d(TAG, "GetFBConfigs: screen=" + screen);

        int defaultVisualId = getDefaultVisualId();
        int numFBConfigs = 1;
        int numProps = 28;
        int[] config = buildVisualConfig(defaultVisualId);
        int replyLength = numFBConfigs * numProps;

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(replyLength);
            outputStream.writeInt(numFBConfigs);
            outputStream.writeInt(numProps);  // required by GLX protocol — was missing (treated as 0)
            outputStream.writePad(16);
            for (int prop : config) {
                outputStream.writeInt(prop);
            }
        }
    }

    private void handleQueryExtensionsString(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int screen = inputStream.readInt();
        String extensions = "GLX_ARB_multisample GLX_EXT_visual_info GLX_EXT_visual_rating GLX_ARB_create_context GLX_ARB_create_context_profile";
        byte[] extBytes = extensions.getBytes();
        int padded = (extBytes.length + 3) & ~3;

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt((padded + 4) / 4);
            outputStream.writePad(4);
            outputStream.writeInt(extBytes.length);
            outputStream.writePad(16);
            outputStream.write(extBytes);
            if (padded > extBytes.length) {
                outputStream.writePad(padded - extBytes.length);
            }
        }
    }

    private void handleQueryServerString(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int screen = inputStream.readInt();
        int name = inputStream.readInt();
        String result;
        switch (name) {
            case 1: result = "GameNative GL4ES"; break;
            case 2: result = "1.4"; break;
            case 3: result = "GLX_ARB_multisample GLX_EXT_visual_info GLX_ARB_create_context"; break;
            default: result = ""; break;
        }
        byte[] strBytes = result.getBytes();
        int padded = (strBytes.length + 3) & ~3;

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt((padded + 4) / 4);
            outputStream.writePad(4);
            outputStream.writeInt(strBytes.length);
            outputStream.writePad(16);
            outputStream.write(strBytes);
            if (padded > strBytes.length) {
                outputStream.writePad(padded - strBytes.length);
            }
        }
    }

    /**
     * GLX IsDirect — responds that the context is direct (rendering goes to EGL/ANGLE
     * client-side, not through an indirect X11 protocol path).
     */
    private void handleIsDirect(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int context = inputStream.readInt();
        Log.d(TAG, "IsDirect: context=0x" + Integer.toHexString(context) + " → True (direct via GL4ES/ANGLE)");

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeByte((byte) 1); // isDirect = True
            outputStream.writePad(23);
        }
    }

    /**
     * GLX MakeCurrent — responds with a context tag.
     * GL4ES handles the actual EGL context switch client-side.
     */
    private void handleMakeCurrent(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int drawable = inputStream.readInt();
        int context = inputStream.readInt();
        int oldContextTag = inputStream.readInt();
        int tag = nextContextTag++;
        Log.d(TAG, "MakeCurrent: drawable=0x" + Integer.toHexString(drawable)
            + " context=0x" + Integer.toHexString(context)
            + " oldTag=" + oldContextTag + " → newTag=" + tag);

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeInt(tag);
            outputStream.writePad(20);
        }
    }

    /**
     * GLX MakeContextCurrent (GLX 1.3) — separate draw/read drawables.
     */
    private void handleMakeContextCurrent(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int oldContextTag = inputStream.readInt();
        int drawable = inputStream.readInt();
        int readDrawable = inputStream.readInt();
        int context = inputStream.readInt();
        int tag = nextContextTag++;
        Log.d(TAG, "MakeContextCurrent: draw=0x" + Integer.toHexString(drawable)
            + " read=0x" + Integer.toHexString(readDrawable)
            + " context=0x" + Integer.toHexString(context) + " → tag=" + tag);

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeInt(tag);
            outputStream.writePad(20);
        }
    }

    private void handleQueryContext(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int context = inputStream.readInt();
        Log.d(TAG, "QueryContext: context=0x" + Integer.toHexString(context));

        int numAttribs = 4;
        int defaultVisualId = getDefaultVisualId();

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(numAttribs * 2);
            outputStream.writeInt(numAttribs);
            outputStream.writePad(20);
            // GLX_FBCONFIG_ID (0x8013)
            outputStream.writeInt(0x8013);
            outputStream.writeInt(defaultVisualId);
            // GLX_VISUAL_ID (0x800B)
            outputStream.writeInt(0x800B);
            outputStream.writeInt(defaultVisualId);
            // GLX_SCREEN (0x800C)
            outputStream.writeInt(0x800C);
            outputStream.writeInt(0);
            // GLX_RENDER_TYPE (0x8011) = GLX_RGBA_BIT (1)
            outputStream.writeInt(0x8011);
            outputStream.writeInt(1);
        }
    }

    private void handleGetDrawableAttributes(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int drawable = inputStream.readInt();
        Log.d(TAG, "GetDrawableAttributes: drawable=0x" + Integer.toHexString(drawable));

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writeInt(0); // num attributes
            outputStream.writePad(20);
        }
    }

    private void handleVendorPrivateWithReply(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int vendorCode = inputStream.readInt();
        Log.d(TAG, "VendorPrivateWithReply: vendorCode=" + vendorCode + " — returning empty reply");
        client.skipRequest();

        try (XStreamLock lock = outputStream.lock()) {
            outputStream.writeByte(RESPONSE_CODE_SUCCESS);
            outputStream.writeByte((byte) 0);
            outputStream.writeShort(client.getSequenceNumber());
            outputStream.writeInt(0);
            outputStream.writePad(24);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // No-reply handlers
    // ═══════════════════════════════════════════════════════════════════════

    private void handleCreateContext(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int context = inputStream.readInt();
        int visual = inputStream.readInt();
        int screen = inputStream.readInt();
        int shareList = inputStream.readInt();
        boolean isDirect = inputStream.readByte() != 0;
        inputStream.skip(3);
        // Consume any unexpected trailing bytes to keep the X11 stream in sync.
        client.skipRequest();
        Log.i(TAG, "CreateContext: ctx=0x" + Integer.toHexString(context)
            + " visual=" + visual + " screen=" + screen
            + " share=0x" + Integer.toHexString(shareList)
            + " direct=" + isDirect
            + " (GL4ES handles real context via EGL/ANGLE)");
    }

    private void handleCreateNewContext(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int context = inputStream.readInt();
        int fbconfig = inputStream.readInt();
        int screen = inputStream.readInt();
        int renderType = inputStream.readInt();
        int shareList = inputStream.readInt();
        boolean isDirect = inputStream.readByte() != 0;
        inputStream.skip(3);
        client.skipRequest();
        Log.i(TAG, "CreateNewContext: ctx=0x" + Integer.toHexString(context)
            + " fbconfig=" + fbconfig + " renderType=" + renderType
            + " direct=" + isDirect);
    }

    private void handleCreateContextAttribsARB(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int context = inputStream.readInt();
        int fbconfig = inputStream.readInt();
        int screen = inputStream.readInt();
        int shareList = inputStream.readInt();
        boolean isDirect = inputStream.readByte() != 0;
        inputStream.skip(3);
        int numAttribs = inputStream.readInt();
        Log.i(TAG, "CreateContextAttribsARB: ctx=0x" + Integer.toHexString(context)
            + " fbconfig=" + fbconfig + " numAttribs=" + numAttribs);
        for (int i = 0; i < numAttribs; i++) {
            inputStream.readInt();
            inputStream.readInt();
        }
        // Consume any remaining bytes (e.g. padding) to keep the stream in sync.
        client.skipRequest();
    }

    private void handleDestroyContext(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int context = inputStream.readInt();
        client.skipRequest();
        Log.d(TAG, "DestroyContext: ctx=0x" + Integer.toHexString(context));
    }

    private void handleSwapBuffers(XClient client, XInputStream inputStream, XOutputStream outputStream) throws IOException {
        int contextTag = inputStream.readInt();
        int drawable = inputStream.readInt();
        client.skipRequest();
        Log.d(TAG, "SwapBuffers: tag=" + contextTag + " drawable=0x" + Integer.toHexString(drawable));
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Helpers
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * Build a 28-property GLX visual/FBConfig array matching the canonical GLX 1.4
     * property order expected by Wine's winex11.drv glx pixel format parser.
     *
     * Wine reads specific offsets for GLX_FBCONFIG_ID, GLX_VISUAL_ID, GLX_DRAWABLE_TYPE,
     * GLX_RENDER_TYPE, GLX_X_RENDERABLE, etc. Zero values for GLX_DRAWABLE_TYPE or
     * GLX_RENDER_TYPE cause Wine to reject the config → 0 pixel formats → adapter_no3d.
     *
     * Property layout (GLX protocol, 28 properties per visual):
     *  0: visualID           1: class              2: rgba
     *  3: redSize            4: greenSize          5: blueSize
     *  6: alphaSize          7: accumRedSize       8: accumGreenSize
     *  9: accumBlueSize     10: accumAlphaSize    11: doubleBuffer
     * 12: stereo            13: bufferSize        14: depthSize
     * 15: stencilSize       16: auxBuffers        17: level
     * 18: GLX_FBCONFIG_ID   19: GLX_VISUAL_ID     20: GLX_X_VISUAL_TYPE
     * 21: GLX_CONFIG_CAVEAT 22: GLX_TRANSPARENT_TYPE
     * 23: GLX_DRAWABLE_TYPE 24: GLX_RENDER_TYPE   25: GLX_X_RENDERABLE
     * 26: GLX_MAX_PBUFFER_WIDTH  27: GLX_MAX_PBUFFER_HEIGHT
     */
    private int[] buildVisualConfig(int visualId) {
        int numProps = 28;
        int[] config = new int[numProps];
        config[0] = visualId;   // visualID
        config[1] = 4;          // class = TrueColor
        config[2] = 1;          // rgba = True
        config[3] = 8;          // redSize
        config[4] = 8;          // greenSize
        config[5] = 8;          // blueSize
        config[6] = 8;          // alphaSize
        config[7] = 0;          // accumRedSize
        config[8] = 0;          // accumGreenSize
        config[9] = 0;          // accumBlueSize
        config[10] = 0;         // accumAlphaSize
        config[11] = 1;         // doubleBuffer
        config[12] = 0;         // stereo
        config[13] = 32;        // bufferSize
        config[14] = 24;        // depthSize
        config[15] = 8;         // stencilSize
        config[16] = 0;         // auxBuffers
        config[17] = 0;         // level
        // ── Extended properties (indices 18–27) ──
        // These MUST be non-zero for Wine to accept the config.
        config[18] = visualId;  // GLX_FBCONFIG_ID — unique config identifier
        config[19] = visualId;  // GLX_VISUAL_ID — associated X visual
        config[20] = 0x8002;    // GLX_X_VISUAL_TYPE = GLX_TRUE_COLOR (0x8002)
        config[21] = 0x8000;    // GLX_CONFIG_CAVEAT = GLX_NONE (0x8000)
        config[22] = 0x8000;    // GLX_TRANSPARENT_TYPE = GLX_NONE (0x8000)
        config[23] = 0x0001 | 0x0004; // GLX_DRAWABLE_TYPE = GLX_WINDOW_BIT | GLX_PBUFFER_BIT
        config[24] = 0x0001;    // GLX_RENDER_TYPE = GLX_RGBA_BIT
        config[25] = 1;         // GLX_X_RENDERABLE = True
        config[26] = 4096;      // GLX_MAX_PBUFFER_WIDTH
        config[27] = 4096;      // GLX_MAX_PBUFFER_HEIGHT
        return config;
    }

    private int getDefaultVisualId() {
        if (xServer != null && xServer.pixmapManager != null && xServer.pixmapManager.visual != null) {
            return xServer.pixmapManager.visual.id;
        }
        return 1;
    }

    private static String subOpcodeName(int opcode) {
        switch (opcode) {
            case GLX_RENDER: return "Render";
            case GLX_RENDER_LARGE: return "RenderLarge";
            case GLX_CREATE_CONTEXT: return "CreateContext";
            case GLX_DESTROY_CONTEXT: return "DestroyContext";
            case GLX_MAKE_CURRENT: return "MakeCurrent";
            case GLX_IS_DIRECT: return "IsDirect";
            case GLX_QUERY_VERSION: return "QueryVersion";
            case GLX_WAIT_GL: return "WaitGL";
            case GLX_WAIT_X: return "WaitX";
            case GLX_COPY_CONTEXT: return "CopyContext";
            case GLX_SWAP_BUFFERS: return "SwapBuffers";
            case GLX_USE_X_FONT: return "UseXFont";
            case GLX_CREATE_GLX_PIXMAP: return "CreateGLXPixmap";
            case GLX_GET_VISUAL_CONFIGS: return "GetVisualConfigs";
            case GLX_DESTROY_GLX_PIXMAP: return "DestroyGLXPixmap";
            case GLX_VENDOR_PRIVATE: return "VendorPrivate";
            case GLX_VENDOR_PRIVATE_WITH_REPLY: return "VendorPrivateWithReply";
            case GLX_QUERY_EXTENSIONS_STRING: return "QueryExtensionsString";
            case GLX_QUERY_SERVER_STRING: return "QueryServerString";
            case GLX_CLIENT_INFO: return "ClientInfo";
            case GLX_GET_FB_CONFIGS: return "GetFBConfigs";
            case GLX_CREATE_PIXMAP: return "CreatePixmap";
            case GLX_DESTROY_PIXMAP: return "DestroyPixmap";
            case GLX_CREATE_NEW_CONTEXT: return "CreateNewContext";
            case GLX_QUERY_CONTEXT: return "QueryContext";
            case GLX_MAKE_CONTEXT_CURRENT: return "MakeContextCurrent";
            case GLX_CREATE_PBUFFER: return "CreatePbuffer";
            case GLX_DESTROY_PBUFFER: return "DestroyPbuffer";
            case GLX_GET_DRAWABLE_ATTRIBUTES: return "GetDrawableAttributes";
            case GLX_CHANGE_DRAWABLE_ATTRIBUTES: return "ChangeDrawableAttributes";
            case GLX_CREATE_WINDOW: return "CreateWindow";
            case GLX_DESTROY_WINDOW: return "DestroyWindow";
            case GLX_SET_CLIENT_INFO_ARB: return "SetClientInfoARB";
            case GLX_CREATE_CONTEXT_ATTRIBS_ARB: return "CreateContextAttribsARB";
            case GLX_SET_CLIENT_INFO_2_ARB: return "SetClientInfo2ARB";
            default: return "Unknown(" + opcode + ")";
        }
    }
}
