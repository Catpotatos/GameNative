package com.winlator.xenvironment.components;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import com.winlator.PrefManager;
import com.winlator.xenvironment.EnvironmentComponent;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import app.gamenative.ui.screen.auth.EpicOAuthActivity;

public class WineRequestComponent extends EnvironmentComponent {
    private static final String TAG = "WineRequestComponent";

    abstract class RequestCodes {
        static final int OPEN_URL = 1;
        //static final int GET_WINE_CLIPBOARD = 2;
        //static final int SET_WINE_CLIPBAORD = 3;
    }

    private ServerSocket serverSocket;
    private volatile boolean isRunning = false;
    private ExecutorService executor;

    private boolean openWithAndroidBrowser = false;

    public void start() {
        // Read preference here where environment (and thus context) is available,
        // ensuring PrefManager is initialized before use.
        try {
            PrefManager.init(environment.getContext());
            this.openWithAndroidBrowser = PrefManager.getBoolean("open_web_links_externally", false);
        } catch (Exception e) {
            Log.w(TAG, "Failed to read open_web_links_externally preference, defaulting to false", e);
            this.openWithAndroidBrowser = false;
        }

        isRunning = true;
        executor = Executors.newSingleThreadExecutor();
        executor.execute(() -> {
            try {
                serverSocket = new ServerSocket(20000, 50, InetAddress.getLocalHost());
                Log.d(TAG, "Server socket started on port 20000");
                while (isRunning) {
                    Socket socket = serverSocket.accept();
                    try {
                        DataInputStream inputStream = new DataInputStream(socket.getInputStream());
                        DataOutputStream outputStream = new DataOutputStream(socket.getOutputStream());
                        int requestCode = inputStream.readInt();
                        handleRequest(inputStream, outputStream, requestCode);
                    } finally {
                        // Ensure socket is closed even if handleRequest throws, preventing fd leaks
                        try {
                            socket.close();
                        } catch (IOException ignored) {}
                    }
                }
            } catch (IOException e) {
                if (isRunning) {
                    Log.e(TAG, "Server socket error while running", e);
                }
            }
        });
    }

    public void stop() {
        isRunning = false;
        if (serverSocket != null) {
            try {
                serverSocket.close();
            } catch (IOException e) {
                Log.w(TAG, "Error closing server socket", e);
            }
        }
        if (executor != null) {
            executor.shutdown();
        }
    }

    public void handleRequest(DataInputStream inputStream, DataOutputStream outputStream, int requestCode) throws IOException {
        switch(requestCode) {
            case RequestCodes.OPEN_URL:
                openURL(inputStream, outputStream);
                break;
//            case RequestCodes.GET_WINE_CLIPBOARD:
//                getWineClipboard(inputStream, outputStream);
//                break;
//            case RequestCodes.SET_WINE_CLIPBAORD:
//                setWineClipboard(inputStream, outputStream);
//                break;
        }
    }

    private void openURL(DataInputStream inputStream, DataOutputStream outputStream) throws IOException {
        Context context = environment.getContext();

        int messageLength = inputStream.readInt();
        byte[] data = new byte[messageLength];
        inputStream.readFully(data);
        String url = new String(data, "UTF-8");

        if (url.startsWith(EpicOAuthActivity.EPIC_AUTH_URL_PREFIX)) {
            // handled, start EpicOAuthActivity here, pass the url to EpicOAuthActivity
            Intent intent = new Intent();
            intent.setClass(context, EpicOAuthActivity.class);
            intent.putExtra(EpicOAuthActivity.EXTRA_GAME_AUTH_URL, url);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
            return;
        }

        if (openWithAndroidBrowser) {
            Log.d("WineRequestComponent", "Received request code OPEN_URL with url " + url.substring(0, Math.min(url.length(), 20)));
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        }
    }

}
