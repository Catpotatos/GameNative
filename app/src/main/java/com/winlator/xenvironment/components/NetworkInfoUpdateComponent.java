package com.winlator.xenvironment.components;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.util.Log;

import com.winlator.core.FileUtils;
import com.winlator.core.NetworkHelper;
import com.winlator.xenvironment.EnvironmentComponent;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.io.IOException;
import java.util.List;

public class NetworkInfoUpdateComponent extends EnvironmentComponent {
    private BroadcastReceiver broadcastReceiver;
    private String lastKnownIPv4 = null;
    private List<NetworkHelper.IFAddress> lastKnownIFAddresses = null;

    @Override
    public void start() {
        Log.d("NetworkInfoUpdateComponent", "Starting...");
        Context context = environment.getContext();
        final NetworkHelper networkHelper = new NetworkHelper(context);
        List<NetworkHelper.IFAddress> ifAddresses = networkHelper.getIFAddresses();
        String ipv4Address = networkHelper.getIPv4Address();

        // Store initial state
        lastKnownIPv4 = ipv4Address;
        lastKnownIFAddresses = ifAddresses;

        updateIFAddrsFile(ifAddresses);
        updateEtcHostsFile(ipv4Address);
        this.broadcastReceiver = new BroadcastReceiver() { // from class: com.winlator.xenvironment.components.NetworkInfoUpdateComponent.1
            @Override // android.content.BroadcastReceiver
            public void onReceive(Context context2, Intent intent) {
                Log.d("NetworkInfoUpdateComponent", "Network connectivity event received");
                try {
                    List<NetworkHelper.IFAddress> ifAddresses = networkHelper.getIFAddresses();
                    String ipv4Address = networkHelper.getIPv4Address();

                    // Only update if the state actually changed - avoid file thrashing from duplicate events
                    boolean ifAddressesChanged = hasIFAddressesChanged(ifAddresses);
                    boolean ipv4Changed = hasIPv4Changed(ipv4Address);

                    if (!ifAddressesChanged && !ipv4Changed) {
                        Log.d("NetworkInfoUpdateComponent", "Network state unchanged, skipping file update");
                        return;
                    }

                    Log.d("NetworkInfoUpdateComponent", "Network state changed (ifAddr=" + ifAddressesChanged +
                          ", ipv4=" + ipv4Changed + "), updating configuration");

                    if (ifAddressesChanged) {
                        if (ifAddresses == null) {
                            Log.w("NetworkInfoUpdateComponent", "getIFAddresses() returned null");
                        } else {
                            updateIFAddrsFile(ifAddresses);
                            lastKnownIFAddresses = ifAddresses;
                        }
                    }

                    if (ipv4Changed) {
                        updateEtcHostsFile(ipv4Address);
                        lastKnownIPv4 = ipv4Address;
                    }
                } catch (Exception e) {
                    Log.e("NetworkInfoUpdateComponent", "Error updating network configuration: " + e, e);
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                context.registerReceiver(this.broadcastReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
            } else {
                context.registerReceiver(this.broadcastReceiver, filter);
            }
            Log.d("NetworkInfoUpdateComponent", "Broadcast receiver registered successfully");
        } catch (Exception e) {
            Log.e("NetworkInfoUpdateComponent", "Failed to register broadcast receiver: " + e, e);
            this.broadcastReceiver = null;
        }
    }

    @Override
    public void stop() {
        Log.d("NetworkInfoUpdateComponent", "Stopping...");
        if (broadcastReceiver != null) {
            try {
                Context context = environment.getContext();
                if (context != null) {
                    context.unregisterReceiver(broadcastReceiver);
                    Log.d("NetworkInfoUpdateComponent", "Broadcast receiver unregistered successfully");
                } else {
                    Log.w("NetworkInfoUpdateComponent", "Context is null, cannot unregister receiver");
                }
            } catch(Exception e) {
                Log.e("NetworkInfoUpdateComponent", "Failed to unregister broadcast receiver: " + e, e);
            }
            broadcastReceiver = null;
        }
    }

    private void updateAdapterInfoFile(int ipAddress, int netmask, int gateway) {
        File file = new File(environment.getImageFs().getTmpDir(), "adapterinfo");
        FileUtils.writeString(file, "Android Wi-Fi Adapter,"+NetworkHelper.formatIpAddress(ipAddress)+","+NetworkHelper.formatNetmask(netmask)+","+NetworkHelper.formatIpAddress(gateway));
    }

    public void updateIFAddrsFile(List<NetworkHelper.IFAddress> ifAddresses) {
        if (ifAddresses == null) {
            Log.w("NetworkInfoUpdateComponent", "ifAddresses is null, skipping update");
            return;
        }

        File file = new File(environment.getImageFs().getTmpDir(), "ifaddrs");
        String content = "";
        if (!ifAddresses.isEmpty()) {
            for (NetworkHelper.IFAddress ifAddress : ifAddresses) {
                StringBuilder sb = new StringBuilder();
                sb.append(content);
                sb.append(!content.isEmpty() ? "\n" : "");
                sb.append(ifAddress.toString());
                content = sb.toString();
            }
        } else {
            content = new NetworkHelper.IFAddress().toString();
        }

        boolean success = FileUtils.writeString(file, content);
        if (!success) {
            Log.e("NetworkInfoUpdateComponent", "Failed to write ifaddrs file: " + file.getAbsolutePath());
        } else {
            Log.d("NetworkInfoUpdateComponent", "Successfully updated ifaddrs file");
        }
    }

    static String buildEtcHostsContent(String ipAddress) {
        StringBuilder content = new StringBuilder()
                .append("127.0.0.1\tlocalhost\n")
                .append("::1\tip6-localhost ip6-loopback\n")
                .append("fe00::0\tip6-localnet\n");

        // NetworkHelper supplies an IPv4 literal. Reject whitespace/control characters so a
        // malformed value cannot inject another hosts entry.
        if (ipAddress != null) {
            String ip = ipAddress.trim();
            if (!ip.isEmpty() && ip.matches("[0-9.]+") && !ip.equals("127.0.0.1")) {
                content.append(ip).append("\tandroid-host\n");
            }
        }
        return content.toString();
    }

    public void updateEtcHostsFile(String ipAddress) {
        File file = new File(environment.getImageFs().getRootDir(), "etc/hosts");
        String content = buildEtcHostsContent(ipAddress);

        // Check if file content is already up-to-date to avoid unnecessary writes
        byte[] current = FileUtils.read(file);
        if (current != null && content.equals(new String(current, StandardCharsets.UTF_8))) {
            Log.d("NetworkInfoUpdateComponent", "etc/hosts already up-to-date, skipping write");
            return;
        }

        boolean success = FileUtils.writeString(file, content);
        if (!success) {
            Log.e("NetworkInfoUpdateComponent", "Failed to write etc/hosts file: " + file.getAbsolutePath() +
                  " (ipAddress=" + ipAddress + ")");
        } else {
            Log.d("NetworkInfoUpdateComponent", "Successfully updated etc/hosts file with IP: " + ipAddress);
        }
    }

    private boolean hasIPv4Changed(String currentIPv4) {
        // Return true if IP actually changed
        if (lastKnownIPv4 == null && currentIPv4 == null) {
            return false; // Both null, no change
        }
        if (lastKnownIPv4 == null || currentIPv4 == null) {
            return true; // One is null, other isn't - change detected
        }
        return !lastKnownIPv4.equals(currentIPv4); // Compare actual values
    }

    private boolean hasIFAddressesChanged(List<NetworkHelper.IFAddress> currentAddresses) {
        // Return true if addresses actually changed
        if (lastKnownIFAddresses == null && currentAddresses == null) {
            return false; // Both null, no change
        }
        if (lastKnownIFAddresses == null || currentAddresses == null) {
            return true; // One is null, other isn't - change detected
        }
        if (lastKnownIFAddresses.size() != currentAddresses.size()) {
            return true; // Size changed
        }

        // Compare each address
        for (int i = 0; i < lastKnownIFAddresses.size(); i++) {
            NetworkHelper.IFAddress last = lastKnownIFAddresses.get(i);
            NetworkHelper.IFAddress current = currentAddresses.get(i);
            if (!last.toString().equals(current.toString())) {
                return true; // Content changed
            }
        }
        return false; // No changes detected
    }
}
