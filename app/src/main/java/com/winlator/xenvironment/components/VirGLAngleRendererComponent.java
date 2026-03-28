package com.winlator.xenvironment.components;

import android.util.Log;

import androidx.annotation.Keep;

import com.winlator.renderer.GLRenderer;
import com.winlator.renderer.Texture;
import com.winlator.xconnector.Client;
import com.winlator.xconnector.ConnectionHandler;
import com.winlator.xconnector.RequestHandler;
import com.winlator.xconnector.UnixSocketConfig;
import com.winlator.xconnector.XConnectorEpoll;
import com.winlator.xenvironment.EnvironmentComponent;
import com.winlator.xserver.Drawable;
import com.winlator.xserver.XServer;

import java.io.IOException;

/**
 * VirGL renderer component that uses ANGLE as the host-side EGL/GLES backend
 * instead of the system EGL/GLES driver.
 *
 * The rendering chain:
 *   Guest (GLIBC): Game → WineD3D → OpenGL → Mesa virgl driver → unix socket
 *   Host (Android): virgl_server → ANGLE EGL/GLES → Vulkan ICD → GPU
 *
 * ANGLE provides more consistent GLES 3.x support and extension coverage
 * than vendor GLES drivers, especially on non-Adreno GPUs (Mali, PowerVR).
 *
 * This component loads the 'virglrenderer_angle' JNI library, which is linked
 * against ANGLE's libEGL_angle.so and libGLESv2_angle.so instead of the system
 * EGL/GLESv2. The native code uses eglGetPlatformDisplayEXT() with
 * EGL_PLATFORM_ANGLE_ANGLE to explicitly select ANGLE's Vulkan backend.
 */
public class VirGLAngleRendererComponent extends EnvironmentComponent implements ConnectionHandler, RequestHandler {
    private static final String TAG = "VirGLAngleRenderer";
    private final XServer xServer;
    private final UnixSocketConfig socketConfig;
    private XConnectorEpoll connector;
    private long sharedEGLContextPtr;

    static {
        System.loadLibrary("virglrenderer_angle");
    }

    public VirGLAngleRendererComponent(XServer xServer, UnixSocketConfig socketConfig) {
        this.xServer = xServer;
        this.socketConfig = socketConfig;
    }

    @Override
    public void start() {
        Log.d(TAG, "Starting (ANGLE-backed VirGL)...");
        if (connector != null) return;
        connector = new XConnectorEpoll(socketConfig, this, this);
        connector.start();
    }

    @Override
    public void stop() {
        Log.d(TAG, "Stopping...");
        if (connector != null) {
            connector.stop();
            connector = null;
        }
    }

    @Keep
    private void killConnection(int fd) {
        connector.killConnection(connector.getClient(fd));
    }

    @Keep
    private long getSharedEGLContext() {
        Log.d(TAG, "Calling getSharedEGLContext");
        if (sharedEGLContextPtr != 0) return sharedEGLContextPtr;
        final Thread thread = Thread.currentThread();
        try {
            GLRenderer renderer = xServer.getRenderer();
            renderer.xServerView.queueEvent(() -> {
                sharedEGLContextPtr = getCurrentEGLContextPtr();

                synchronized(thread) {
                    thread.notify();
                }
            });
            synchronized (thread) {
                thread.wait();
            }
        }
        catch (Exception e) {
            return 0;
        }
        Log.d(TAG, "Finished getSharedEGLContext");
        return sharedEGLContextPtr;
    }

    @Override
    public void handleConnectionShutdown(Client client) {
        long clientPtr = (long)client.getTag();
        destroyClient(clientPtr);
    }

    @Override
    public void handleNewConnection(Client client) {
        Log.d(TAG, "Calling handleNewConnection");
        getSharedEGLContext();
        long clientPtr = handleNewConnection(client.clientSocket.fd);
        client.setTag(clientPtr);
        Log.d(TAG, "Finished handleNewConnection");
    }

    @Override
    public boolean handleRequest(Client client) throws IOException {
        Log.d(TAG, "Calling handleRequest");
        long clientPtr = (long)client.getTag();
        if (clientPtr == 0 || !client.isConnected()) return false;
        handleRequest(clientPtr);
        Log.d(TAG, "Finished handleRequest");
        return client.isConnected();
    }

    /**
     * Called from native code (virgl_server_renderer.c) to locate the
     * directory where ANGLE's libEGL.so and libGLESv2.so were extracted.
     * The native side dlopen's them from this path.
     */
    @Keep
    private String getAngleLibPath() {
        // The ANGLE libs are extracted under the imagefs at opt/angle/lib/
        // The imagefs root is at <dataDir>/files/imagefs/
        // Use the app's own files directory to construct the path
        try {
            android.content.Context ctx = (environment != null) ? environment.getContext() : null;
            if (ctx != null) {
                java.io.File filesDir = ctx.getFilesDir();
                java.io.File candidate = new java.io.File(filesDir, "imagefs/opt/angle/lib");
                String path = candidate.getAbsolutePath();
                Log.d(TAG, "getAngleLibPath (from context): " + path);
                return path;
            }
        } catch (Exception e) {
            Log.w(TAG, "Could not get context for ANGLE path", e);
        }
        // Fallback: construct from known data directory structure
        java.io.File dataDir = new java.io.File("/data/data/" + "app.gamenative");
        java.io.File candidate = new java.io.File(dataDir, "files/imagefs/opt/angle/lib");
        if (!candidate.exists()) {
            dataDir = new java.io.File("/data/user/0/" + "app.gamenative");
            candidate = new java.io.File(dataDir, "files/imagefs/opt/angle/lib");
        }
        String path = candidate.getAbsolutePath();
        Log.d(TAG, "getAngleLibPath (fallback): " + path);
        return path;
    }

    @Keep
    private void flushFrontbuffer(int drawableId, int framebuffer) {
        Log.d(TAG, "Calling flushFrontbuffer");
        Drawable drawable = xServer.drawableManager.getDrawable(drawableId);
        if (drawable == null) return;

        synchronized (drawable.renderLock) {
            drawable.setData(null);
            Texture texture = drawable.getTexture();
            texture.copyFromFramebuffer(framebuffer, drawable.width, drawable.height);
        }

        Runnable onDrawListener = drawable.getOnDrawListener();
        if (onDrawListener != null) onDrawListener.run();
        Log.d(TAG, "Finished flushFrontbuffer");
    }

    private native long handleNewConnection(int fd);

    private native void handleRequest(long clientPtr);

    private native long getCurrentEGLContextPtr();

    private native void destroyClient(long clientPtr);

    private native void destroyRenderer(long clientPtr);
}

