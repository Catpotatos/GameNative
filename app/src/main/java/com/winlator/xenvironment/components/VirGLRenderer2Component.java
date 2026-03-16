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
 * VirGLRenderer2Component - Virgl renderer using virglrenderer 1.3.0.
 *
 * This is a newer version of the VirGL renderer that users can select
 * as an alternative to the original VirGL 23.1.9.
 * It loads libvirglrenderer2.so which is built from virglrenderer 1.3.0 standalone.
 */
public class VirGLRenderer2Component extends EnvironmentComponent implements ConnectionHandler, RequestHandler {
    private static final String TAG = "VirGLRenderer2Component";
    private final XServer xServer;
    private final UnixSocketConfig socketConfig;
    private XConnectorEpoll connector;
    private long sharedEGLContextPtr;
    private volatile boolean stopped = false;

    static {
        System.loadLibrary("virglrenderer2");
    }

    public VirGLRenderer2Component(XServer xServer, UnixSocketConfig socketConfig) {
        this.xServer = xServer;
        this.socketConfig = socketConfig;
    }

    @Override
    public void start() {
        Log.d(TAG, "Starting VirGL 1.3.0 renderer...");
        if (connector != null) return;
        stopped = false;
        connector = new XConnectorEpoll(socketConfig, this, this);
        connector.start();
    }

    @Override
    public void stop() {
        Log.d(TAG, "Stopping VirGL 1.3.0 renderer...");
        stopped = true;
        if (connector != null) {
            connector.stop();
            connector = null;
        }
    }

    /**
     * Signal the renderer to stop processing new requests without fully
     * tearing down the connector.
     */
    public void signalStop() {
        Log.d(TAG, "signalStop called");
        stopped = true;
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
        if (stopped) {
            Log.d(TAG, "handleRequest skipped (stopped)");
            throw new IOException("VirGL renderer stopped");
        }
        long clientPtr = (long)client.getTag();
        handleRequest(clientPtr);
        return true;
    }

    @Keep
    private void flushFrontbuffer(int drawableId, int framebuffer) {
        if (stopped) return;
        Drawable drawable = xServer.drawableManager.getDrawable(drawableId);
        if (drawable == null) return;

        synchronized (drawable.renderLock) {
            drawable.setData(null);
            Texture texture = drawable.getTexture();
            texture.copyFromFramebuffer(framebuffer, drawable.width, drawable.height);
        }

        Runnable onDrawListener = drawable.getOnDrawListener();
        if (onDrawListener != null) onDrawListener.run();
    }

    private native long handleNewConnection(int fd);

    private native void handleRequest(long clientPtr);

    private native long getCurrentEGLContextPtr();

    private native void destroyClient(long clientPtr);

    private native void destroyRenderer(long clientPtr);
}

