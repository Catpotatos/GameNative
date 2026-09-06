package com.winlator.core;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkProperties;
import android.net.Network;
import android.util.Log;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;

/**
 * Picks the nameserver that is published to the Wine/Windows side via {@code ANDROID_RESOLV_DNS}.
 *
 * <p>Why not using {@code LinkProperties.getDnsServers().get(0)}:
 *
 * <ul>
 *   <li><b>Most Windows apps never use it.</b> They call {@code getaddrinfo} through ws2_32, which
 *       Wine forwards to bionic and then to Android's {@code netd}. That path works regardless of
 *       what we publish here.</li>
 *   <li><b>Some apps run their own resolver.</b> EA's DirtySDK (EA App / EA Desktop), parts of the
 *       Epic launcher, and several game engines read the configured nameserver via
 *       {@code GetNetworkParams}/{@code iphlpapi} and then send their own UDP/53 queries to it.
 *       For those, this value is the <em>only</em> thing standing between them and
 *       "ERROR_DNS(-1) DNS failure".</li>
 * </ul>
 *
 * <p>Two ways the {@code get(0)} breaks in practice:
 *
 * <ol>
 *   <li>The first entry is an <b>IPv6</b> address. {@code InetAddress.toString()} yields
 *       {@code /fe80::1%wlan0}; stripping the leading slash leaves a scoped literal that the
 *       Windows-side resolver cannot parse.</li>
 *   <li>The first entry is a <b>router that does not actually answer UDP/53</b>. This is common on
 *       phone hotspots and 4G/travel routers (the {@code 192.168.8.0/24} family in particular),
 *       where DHCP hands out the gateway as the resolver but the gateway only proxies DNS for its
 *       own DHCP clients, or Android is using Private DNS (DoT, port 853) and never speaks plain
 *       UDP/53 to it at all.</li>
 * </ol>
 *
 * <p>So, an ordered candidate list (link IPv4 servers first, then public resolvers), probe
 * each one with a real DNS query, and publish the first that answers. The probe is bound to the
 * active {@link Network} so it leaves via the same interface Wine's traffic will.
 */
public final class DnsResolverPicker {
    private static final String TAG = "DnsResolverPicker";

    /** Public fallbacks, tried in order, after anything the link advertises. */
    private static final String[] PUBLIC_FALLBACKS = {"1.1.1.1", "8.8.8.8", "8.8.4.4", "9.9.9.9"};

    /** Last-resort value if literally nothing answers (e.g. probing itself is blocked). */
    private static final String LAST_RESORT = "8.8.8.8";

    /** Name used for the probe query. Short, always resolvable, never NXDOMAIN. */
    private static final String PROBE_NAME = "a.root-servers.net";

    private static final int PROBE_TIMEOUT_MS = 350;
    /** Hard ceiling on the whole selection, so we never stall container start. */
    private static final long TOTAL_BUDGET_MS = 1500L;

    private DnsResolverPicker() {}

    /**
     * @return an IPv4 dotted-quad string that answered a DNS query, or a sane fallback.
     */
    public static String pick(Context context) {
        final List<String> candidates = buildCandidates(context);
        final String[] result = new String[1];

        // Probe off the calling thread: the launcher may run on the main thread, where any socket
        // use would trip StrictMode's NetworkOnMainThreadException.
        Thread prober = new Thread(() -> {
            Network network = activeNetwork(context);
            for (String candidate : candidates) {
                if (probe(candidate, network)) {
                    result[0] = candidate;
                    return;
                }
            }
        }, "dns-picker");
        prober.setDaemon(true);
        prober.start();
        try {
            prober.join(TOTAL_BUDGET_MS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        if (result[0] != null) {
            Log.i(TAG, "Publishing nameserver " + result[0] + " (candidates=" + candidates + ")");
            return result[0];
        }

        // Nothing answered in budget. Prefer the first link-provided IPv4 if we have one -- it is
        // still more likely to be correct on a captive/NAT'd network than a public resolver.
        String fallback = candidates.isEmpty() ? LAST_RESORT : candidates.get(0);
        Log.w(TAG, "No nameserver answered within " + TOTAL_BUDGET_MS + "ms; falling back to " + fallback);
        return fallback;
    }

    /** Link-advertised IPv4 servers first (deduped, IPv6 dropped), then public fallbacks. */
    private static List<String> buildCandidates(Context context) {
        LinkedHashSet<String> ordered = new LinkedHashSet<>();
        try {
            ConnectivityManager cm =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            Network active = cm != null ? cm.getActiveNetwork() : null;
            LinkProperties lp = active != null ? cm.getLinkProperties(active) : null;
            if (lp != null) {
                for (InetAddress address : lp.getDnsServers()) {
                    // IPv4 only: the Windows-side consumers of ANDROID_RESOLV_DNS expect a
                    // dotted-quad, and a scoped IPv6 literal breaks their parsers outright.
                    if (address instanceof Inet4Address) {
                        ordered.add(address.getHostAddress());
                    } else {
                        Log.i(TAG, "Skipping non-IPv4 link nameserver " + address);
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Could not read link nameservers: " + e);
        }
        for (String fallback : PUBLIC_FALLBACKS) ordered.add(fallback);
        return new ArrayList<>(ordered);
    }

    private static Network activeNetwork(Context context) {
        try {
            ConnectivityManager cm =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            return cm != null ? cm.getActiveNetwork() : null;
        } catch (Exception e) {
            return null;
        }
    }

    /** Sends a real A query and waits for any well-formed reply carrying our transaction id. */
    private static boolean probe(String server, Network network) {
        try (DatagramSocket socket = new DatagramSocket()) {
            if (network != null) {
                // Keep the probe on the same interface Wine will use, otherwise we can validate a
                // resolver that is unreachable once traffic actually flows.
                try {
                    network.bindSocket(socket);
                } catch (Exception e) {
                    Log.i(TAG, "Could not bind probe socket to active network: " + e);
                }
            }
            socket.setSoTimeout(PROBE_TIMEOUT_MS);

            byte[] query = buildQuery(PROBE_NAME);
            InetAddress target = InetAddress.getByName(server);
            socket.send(new DatagramPacket(query, query.length, target, 53));

            byte[] buffer = new byte[512];
            DatagramPacket reply = new DatagramPacket(buffer, buffer.length);
            socket.receive(reply);

            // Header is 12 bytes; match the transaction id we generated (0x4741, "GA").
            boolean ok = reply.getLength() >= 12
                    && buffer[0] == (byte) 0x47
                    && buffer[1] == (byte) 0x41;
            if (!ok) Log.i(TAG, "Malformed DNS reply from " + server);
            return ok;
        } catch (Exception e) {
            Log.i(TAG, "Nameserver " + server + " did not answer: " + e);
            return false;
        }
    }

    /** Minimal DNS query packet: header + QNAME + QTYPE(A) + QCLASS(IN). */
    private static byte[] buildQuery(String name) {
        String[] labels = name.split("\\.");
        int qnameLength = 1; // trailing root label
        for (String label : labels) qnameLength += 1 + label.length();

        byte[] packet = new byte[12 + qnameLength + 4];
        int i = 0;
        packet[i++] = 0x47;           // transaction id hi
        packet[i++] = 0x41;           // transaction id lo
        packet[i++] = 0x01;           // flags: standard query, recursion desired
        packet[i++] = 0x00;
        packet[i++] = 0x00;
        packet[i++] = 0x01;           // QDCOUNT = 1
        i += 6;                       // ANCOUNT / NSCOUNT / ARCOUNT all zero

        for (String label : labels) {
            packet[i++] = (byte) label.length();
            for (int c = 0; c < label.length(); c++) packet[i++] = (byte) label.charAt(c);
        }
        packet[i++] = 0x00;           // root label

        packet[i++] = 0x00;
        packet[i++] = 0x01;           // QTYPE = A
        packet[i++] = 0x00;
        packet[i] = 0x01;             // QCLASS = IN
        return packet;
    }
}

