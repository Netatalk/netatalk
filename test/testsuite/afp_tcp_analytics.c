/*
 * Copyright (c) 2026, Andy Lemin (andylemin)
 * Credits; Based on work by Netatalk contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "afp_tcp_analytics.h"

#include <stdio.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "dsi.h"

/* TCP metrics via getsockopt()
 * Supported: Linux, FreeBSD, OpenBSD, NetBSD (TCP_INFO), macOS (TCP_CONNECTION_INFO)
 * Unsupported fields set to -1 and skipped in output */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define HAS_TCP_INFO 1

/* Detect which tcp_info fields are available based on kernel/platform
 * These fields don't exist on Alpine Linux and older kernels */
#if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 17
#define HAS_TCPI_RCV_WND 1
#define HAS_TCPI_REORDERING 1
#define HAS_TCPI_LOST 1
#define HAS_TCPI_RCV_SPACE 1
#endif

#elif defined(__APPLE__)
#define HAS_TCP_CONNECTION_INFO 1
/* macOS uses TCP_CONNECTION_INFO instead of TCP_INFO */
#ifndef TCP_CONNECTION_INFO
#define TCP_CONNECTION_INFO 0x106  /* From sys/socket.h on macOS */
#endif
#endif

#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE*KILOBYTE)

/*!
 * @brief Initialize TCP analytics session
 * @note Must be called before any other tcp_analytics functions
*/
void tcp_analytics_init(TcpAnalyticsSession *session, TcpOutputFormat format,
                        int enable_session)
{
    memset(session, 0, sizeof(*session));
    session->format = format;
    session->enable_session_tracking = enable_session;
}

/*!
 * @brief Capture TCP metrics from connection socket via getsockopt()
 * @note Supports Linux, FreeBSD, NetBSD, OpenBSD (TCP_INFO), macOS (TCP_CONNECTION_INFO)
*/
int tcp_analytics_capture(CONN *conn, TcpMetrics *metrics)
{
    if (!conn) {
        return -1;
    }

    const DSI *conn_dsi = &conn->dsi;
    int sock = conn_dsi->socket;

    if (sock < 0) {
        return -1;
    }

    socklen_t optlen;
    memset(metrics, 0, sizeof(*metrics));
    /* Server quantum from DSI negotiation */
    metrics->server_quantum = conn_dsi->server_quantum;
    /* TCP send buffer */
    optlen = sizeof(metrics->tcp_send_buffer);

    if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                   &metrics->tcp_send_buffer, &optlen) < 0) {
        metrics->tcp_send_buffer = -1;
    }

    /* TCP receive buffer */
    optlen = sizeof(metrics->tcp_recv_buffer);

    if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF,
                   &metrics->tcp_recv_buffer, &optlen) < 0) {
        metrics->tcp_recv_buffer = -1;
    }

    /* TCP MSS */
    optlen = sizeof(metrics->tcp_mss);

    if (getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG,
                   &metrics->tcp_mss, &optlen) < 0) {
        metrics->tcp_mss = -1;
    }

#ifdef HAS_TCP_CONNECTION_INFO
    /* macOS TCP_CONNECTION_INFO */
    struct tcp_connection_info conn_info;
    memset(&conn_info, 0, sizeof(conn_info));
    optlen = sizeof(conn_info);

    if (getsockopt(sock, IPPROTO_TCP, TCP_CONNECTION_INFO, &conn_info,
                   &optlen) == 0) {
        /* Map macOS tcp_connection_info fields to our metrics */
        metrics->tcp_congestion_window = conn_info.tcpi_snd_cwnd;
        metrics->tcp_slow_start_threshold = conn_info.tcpi_snd_ssthresh;
        metrics->tcp_rtt = conn_info.tcpi_srtt;       /* Smoothed RTT in microseconds */
        metrics->tcp_rtt_variance = conn_info.tcpi_rttvar;
        metrics->tcp_retrans = conn_info.tcpi_txretransmitpackets;
        metrics->tcp_total_retrans = -1;  /* Not directly available */
        metrics->tcp_reordering = -1;     /* Not available on macOS */
        metrics->tcp_lost = -1;           /* Not available on macOS */
        metrics->tcp_send_mss = conn_info.tcpi_maxseg;
        metrics->tcp_recv_mss = -1;       /* Not available on macOS */
        metrics->tcp_adv_mss = -1;        /* Not available on macOS */
        metrics->tcp_recv_space = -1;     /* Not available on macOS */
    } else {
        /* getsockopt failed - set all to defaults */
        metrics->tcp_congestion_window = -1;
        metrics->tcp_slow_start_threshold = -1;
        metrics->tcp_rtt = 0;
        metrics->tcp_rtt_variance = 0;
        metrics->tcp_retrans = -1;
        metrics->tcp_total_retrans = -1;
        metrics->tcp_reordering = -1;
        metrics->tcp_lost = -1;
        metrics->tcp_send_mss = -1;
        metrics->tcp_recv_mss = -1;
        metrics->tcp_adv_mss = -1;
        metrics->tcp_recv_space = -1;
    }

#elif defined(HAS_TCP_INFO)
    /* Linux/FreeBSD TCP_INFO
     * Strategy: Use runtime size checking to handle fields that may not exist on older kernels.
     * The actual structure size returned by getsockopt tells us which fields are available.
     */
    struct tcp_info tcp_info;
    memset(&tcp_info, 0, sizeof(tcp_info));
    optlen = sizeof(tcp_info);

    if (getsockopt(sock, IPPROTO_TCP, TCP_INFO, &tcp_info, &optlen) == 0) {
        /* Macro to safely extract fields based on structure size returned by kernel */
#define TCPI_SAFE_GET(field) \
            ((optlen >= ((char*)&tcp_info.field - (char*)&tcp_info) + sizeof(tcp_info.field)))
        /* Core window/congestion metrics - available on most systems
         * NOTE: tcpi_snd_wnd (actual window size) doesn't exist on Alpine/Debian.
         * Using tcpi_snd_cwnd (congestion window) which is universally available. */
        metrics->tcp_congestion_window = TCPI_SAFE_GET(tcpi_snd_cwnd) ?
                                         tcp_info.tcpi_snd_cwnd : -1;
        metrics->tcp_slow_start_threshold = TCPI_SAFE_GET(tcpi_snd_ssthresh) ?
                                            tcp_info.tcpi_snd_ssthresh : -1;
        /* RTT metrics - widely available on Linux/FreeBSD */
        metrics->tcp_rtt = TCPI_SAFE_GET(tcpi_rtt) ? tcp_info.tcpi_rtt : 0;
        metrics->tcp_rtt_variance = TCPI_SAFE_GET(tcpi_rttvar) ? tcp_info.tcpi_rttvar :
                                    0;
        /* Retransmission metrics - available on most systems
         * Use -1 to indicate "not supported" so we can distinguish from "0 retrans" (valid)
         * Note: FreeBSD/NetBSD use __ prefix for some fields */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        metrics->tcp_retrans = TCPI_SAFE_GET(__tcpi_retrans) ? (int)
                               tcp_info.__tcpi_retrans : -1;
        metrics->tcp_total_retrans = -1;  /* Not available on BSDs */
#else
        metrics->tcp_retrans = TCPI_SAFE_GET(tcpi_retrans) ? (int)
                               tcp_info.tcpi_retrans : -1;
        metrics->tcp_total_retrans = TCPI_SAFE_GET(tcpi_total_retrans) ?
                                     (int)tcp_info.tcpi_total_retrans : -1;
#endif
        /* Loss/reordering metrics - may not exist on older kernels
         * Note: FreeBSD/NetBSD use __ prefix for some fields */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        metrics->tcp_reordering = TCPI_SAFE_GET(__tcpi_reordering) ?
                                  (int)tcp_info.__tcpi_reordering : -1;
        metrics->tcp_lost = TCPI_SAFE_GET(__tcpi_lost) ? (int)tcp_info.__tcpi_lost : -1;
#else
        metrics->tcp_reordering = TCPI_SAFE_GET(tcpi_reordering) ?
                                  (int)tcp_info.tcpi_reordering : -1;
        metrics->tcp_lost = TCPI_SAFE_GET(tcpi_lost) ? (int)tcp_info.tcpi_lost : -1;
#endif
        /* MSS metrics - widely available
         * Note: FreeBSD uses __ prefix for advmss */
        metrics->tcp_send_mss = TCPI_SAFE_GET(tcpi_snd_mss) ? tcp_info.tcpi_snd_mss :
                                -1;
        metrics->tcp_recv_mss = TCPI_SAFE_GET(tcpi_rcv_mss) ? tcp_info.tcpi_rcv_mss :
                                -1;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        metrics->tcp_adv_mss = TCPI_SAFE_GET(__tcpi_advmss) ? tcp_info.__tcpi_advmss :
                               -1;
#else
        metrics->tcp_adv_mss = TCPI_SAFE_GET(tcpi_advmss) ? tcp_info.tcpi_advmss : -1;
#endif
        /* Receive space - may not exist on older kernels */
        metrics->tcp_recv_space = TCPI_SAFE_GET(tcpi_rcv_space) ?
                                  tcp_info.tcpi_rcv_space : -1;
#undef TCPI_SAFE_GET
    } else {
        /* getsockopt failed - set all to defaults */
        metrics->tcp_congestion_window = -1;
        metrics->tcp_slow_start_threshold = -1;
        metrics->tcp_rtt = 0;
        metrics->tcp_rtt_variance = 0;
        metrics->tcp_retrans = -1;
        metrics->tcp_total_retrans = -1;
        metrics->tcp_reordering = -1;
        metrics->tcp_lost = -1;
        metrics->tcp_send_mss = -1;
        metrics->tcp_recv_mss = -1;
        metrics->tcp_adv_mss = -1;
        metrics->tcp_recv_space = -1;
    }

#else
    /* TCP_INFO not available on this platform */
    metrics->tcp_congestion_window = -1;
    metrics->tcp_slow_start_threshold = -1;
    metrics->tcp_rtt = 0;
    metrics->tcp_rtt_variance = 0;
    metrics->tcp_retrans = -1;
    metrics->tcp_total_retrans = -1;
    metrics->tcp_reordering = -1;
    metrics->tcp_lost = -1;
    metrics->tcp_send_mss = -1;
    metrics->tcp_recv_mss = -1;
    metrics->tcp_adv_mss = -1;
    metrics->tcp_recv_space = -1;
#endif
    /* Detect localhost */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    optlen = sizeof(addr);

    if (getpeername(sock, (struct sockaddr *)&addr, &optlen) == 0) {
        uint32_t ip = ntohl(addr.sin_addr.s_addr);
        metrics->is_localhost = ((ip >> 24) == 127); /* 127.x.x.x */
    } else {
        metrics->is_localhost = 0;
    }

    return 0;
}

/*!
 * @brief Mark session start - capture "initial" metrics for size sweep mode
*/
int tcp_analytics_session_start(TcpAnalyticsSession *session, CONN *conn)
{
    return tcp_analytics_capture(conn, &session->initial);
}

/*!
 * @brief Mark test start - capture "before" metrics
*/
int tcp_analytics_test_start(TcpAnalyticsSession *session, CONN *conn,
                             off_t file_size)
{
    session->current_size = file_size;
    return tcp_analytics_capture(conn, &session->before);
}

/*!
 * @brief Print before/after TCP metrics comparison for single test
*/
static void print_per_test_comparison(const TcpAnalyticsSession *session)
{
    const TcpMetrics *before = &session->before;
    const TcpMetrics *after = &session->after;
    off_t size = session->current_size;

    if (session->format == TCP_OUTPUT_CSV) {
        return;  /* Suppress table in CSV mode */
    }

    fprintf(stdout, "\n=========== Network & DSI Metrics for File Size: ");

    if (size < MEGABYTE) {
        fprintf(stdout, "%ld KB ===========\n", size / KILOBYTE);
    } else {
        fprintf(stdout, "%ld MB ===========\n", size / MEGABYTE);
    }

    fprintf(stdout, "Connection Type: %s\n",
            before->is_localhost ? "Localhost" : "Network");
    fprintf(stdout, "Server Quantum:  %u bytes (%.1f KB)\n\n",
            before->server_quantum,
            before->server_quantum / 1024.0);
    fprintf(stdout, "%-20s %15s %15s %15s\n", "Metric", "Before", "After", "Delta");
    fprintf(stdout, "%-20s %15s %15s %15s\n", "------", "------", "-----", "-----");

    if (before->tcp_send_buffer > 0) {
        int delta = after->tcp_send_buffer - before->tcp_send_buffer;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d KB", before->tcp_send_buffer / 1024);
        snprintf(a, sizeof(a), "%d KB", after->tcp_send_buffer / 1024);
        snprintf(d, sizeof(d), "%+d KB", delta / 1024);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Send Buffer", b, a, d);
    }

    if (before->tcp_recv_buffer > 0) {
        int delta = after->tcp_recv_buffer - before->tcp_recv_buffer;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d KB", before->tcp_recv_buffer / 1024);
        snprintf(a, sizeof(a), "%d KB", after->tcp_recv_buffer / 1024);
        snprintf(d, sizeof(d), "%+d KB", delta / 1024);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Recv Buffer", b, a, d);
    }

    if (before->tcp_mss > 0) {
        int delta = after->tcp_mss - before->tcp_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", before->tcp_mss);
        snprintf(a, sizeof(a), "%d B", after->tcp_mss);
        snprintf(d, sizeof(d), "%+d B", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP MSS", b, a, d);
    }

    if (before->tcp_congestion_window > 0) {
        int delta = after->tcp_congestion_window - before->tcp_congestion_window;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d", before->tcp_congestion_window);
        snprintf(a, sizeof(a), "%d", after->tcp_congestion_window);
        snprintf(d, sizeof(d), "%+d", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Cwnd (segs)", b, a, d);
    }

    if (before->tcp_slow_start_threshold > 0) {
        int delta = after->tcp_slow_start_threshold - before->tcp_slow_start_threshold;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d", before->tcp_slow_start_threshold);
        snprintf(a, sizeof(a), "%d", after->tcp_slow_start_threshold);
        snprintf(d, sizeof(d), "%+d", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP SSThresh", b, a, d);
    }

    if (before->tcp_rtt > 0) {
        int delta = (int)after->tcp_rtt - (int)before->tcp_rtt;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%.2f ms", before->tcp_rtt / 1000.0);
        snprintf(a, sizeof(a), "%.2f ms", after->tcp_rtt / 1000.0);
        snprintf(d, sizeof(d), "%+.2f ms", delta / 1000.0);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP RTT", b, a, d);
    }

    if (before->tcp_rtt_variance > 0) {
        int delta = (int)after->tcp_rtt_variance - (int)before->tcp_rtt_variance;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%.2f ms", before->tcp_rtt_variance / 1000.0);
        snprintf(a, sizeof(a), "%.2f ms", after->tcp_rtt_variance / 1000.0);
        snprintf(d, sizeof(d), "%+.2f ms", delta / 1000.0);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP RTT Variance", b, a, d);
    }

    /* Retransmissions and losses - only display if supported by kernel */
    if (before->tcp_retrans >= 0) {
        int delta_retrans = after->tcp_retrans - before->tcp_retrans;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d", before->tcp_retrans);
        snprintf(a, sizeof(a), "%d", after->tcp_retrans);
        snprintf(d, sizeof(d), "%+d", delta_retrans);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Retrans", b, a, d);
    }

    if (before->tcp_total_retrans >= 0) {
        int delta_total = after->tcp_total_retrans - before->tcp_total_retrans;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d", before->tcp_total_retrans);
        snprintf(a, sizeof(a), "%d", after->tcp_total_retrans);
        snprintf(d, sizeof(d), "%+d", delta_total);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Total Retrans", b, a, d);
    }

    if (before->tcp_lost >= 0) {
        int delta_lost = after->tcp_lost - before->tcp_lost;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d", before->tcp_lost);
        snprintf(a, sizeof(a), "%d", after->tcp_lost);
        snprintf(d, sizeof(d), "%+d", delta_lost);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Lost", b, a, d);
    }

    if (before->tcp_reordering >= 0) {
        int delta = after->tcp_reordering - before->tcp_reordering;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d", before->tcp_reordering);
        snprintf(a, sizeof(a), "%d", after->tcp_reordering);
        snprintf(d, sizeof(d), "%+d", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Reordering", b, a, d);
    }

    /* MSS metrics */
    if (before->tcp_send_mss > 0) {
        int delta = after->tcp_send_mss - before->tcp_send_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", before->tcp_send_mss);
        snprintf(a, sizeof(a), "%d B", after->tcp_send_mss);
        snprintf(d, sizeof(d), "%+d B", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Send MSS", b, a, d);
    }

    if (before->tcp_recv_mss > 0) {
        int delta = after->tcp_recv_mss - before->tcp_recv_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", before->tcp_recv_mss);
        snprintf(a, sizeof(a), "%d B", after->tcp_recv_mss);
        snprintf(d, sizeof(d), "%+d B", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Recv MSS", b, a, d);
    }

    if (before->tcp_adv_mss > 0) {
        int delta = after->tcp_adv_mss - before->tcp_adv_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", before->tcp_adv_mss);
        snprintf(a, sizeof(a), "%d B", after->tcp_adv_mss);
        snprintf(d, sizeof(d), "%+d B", delta);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Adv MSS", b, a, d);
    }

    if (before->tcp_recv_space > 0) {
        int delta = after->tcp_recv_space - before->tcp_recv_space;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d KB", before->tcp_recv_space / 1024);
        snprintf(a, sizeof(a), "%d KB", after->tcp_recv_space / 1024);
        snprintf(d, sizeof(d), "%+d KB", delta / 1024);
        fprintf(stdout, "%-20s %15s %15s %15s\n", "TCP Recv Space", b, a, d);
    }

    fprintf(stdout,
            "====================================================================\n");
}

/*!
 * @brief Mark test end - capture "after" metrics and print per-test comparison
*/
int tcp_analytics_test_end(TcpAnalyticsSession *session, CONN *conn)
{
    int ret = tcp_analytics_capture(conn, &session->after);

    if (ret < 0) {
        return ret;
    }

    /* Print per-test comparison */
    print_per_test_comparison(session);
    return 0;
}

/*!
 * @brief Print initial/final TCP metrics summary for size sweep session
*/
static void print_session_summary(const TcpAnalyticsSession *session)
{
    const TcpMetrics *initial = &session->initial;
    const TcpMetrics *final = &session->final;

    if (session->format == TCP_OUTPUT_CSV) {
        /* CSV header for TCP summary */
        fprintf(stdout, "\n# TCP Metrics Evolution Summary\n");
        fprintf(stdout, "metric,initial,final,delta\n");

        if (initial->tcp_send_buffer > 0) {
            fprintf(stdout, "tcp_send_buffer_kb,%d,%d,%d\n",
                    initial->tcp_send_buffer / 1024,
                    final->tcp_send_buffer / 1024,
                    (final->tcp_send_buffer - initial->tcp_send_buffer) / 1024);
        }

        if (initial->tcp_recv_buffer > 0) {
            fprintf(stdout, "tcp_recv_buffer_kb,%d,%d,%d\n",
                    initial->tcp_recv_buffer / 1024,
                    final->tcp_recv_buffer / 1024,
                    (final->tcp_recv_buffer - initial->tcp_recv_buffer) / 1024);
        }

        if (initial->tcp_mss > 0) {
            fprintf(stdout, "tcp_mss_bytes,%d,%d,%d\n",
                    initial->tcp_mss,
                    final->tcp_mss,
                    final->tcp_mss - initial->tcp_mss);
        }

        if (initial->tcp_congestion_window >= 0) {
            fprintf(stdout, "tcp_cwnd_segments,%d,%d,%d\n",
                    initial->tcp_congestion_window,
                    final->tcp_congestion_window,
                    final->tcp_congestion_window - initial->tcp_congestion_window);
        }

        if (initial->tcp_slow_start_threshold >= 0) {
            fprintf(stdout, "tcp_ssthresh_segments,%d,%d,%d\n",
                    initial->tcp_slow_start_threshold,
                    final->tcp_slow_start_threshold,
                    final->tcp_slow_start_threshold - initial->tcp_slow_start_threshold);
        }

        if (initial->tcp_rtt > 0) {
            fprintf(stdout, "tcp_rtt_ms,%.2f,%.2f,%.2f\n",
                    initial->tcp_rtt / 1000.0,
                    final->tcp_rtt / 1000.0,
                    ((int)final->tcp_rtt - (int)initial->tcp_rtt) / 1000.0);
        }

        if (initial->tcp_rtt_variance > 0) {
            fprintf(stdout, "tcp_rtt_variance_ms,%.2f,%.2f,%.2f\n",
                    initial->tcp_rtt_variance / 1000.0,
                    final->tcp_rtt_variance / 1000.0,
                    ((int)final->tcp_rtt_variance - (int)initial->tcp_rtt_variance) / 1000.0);
        }

        if (initial->tcp_retrans >= 0) {
            fprintf(stdout, "tcp_retrans_current,%d,%d,%d\n",
                    initial->tcp_retrans,
                    final->tcp_retrans,
                    final->tcp_retrans - initial->tcp_retrans);
        }

        if (initial->tcp_total_retrans >= 0) {
            fprintf(stdout, "tcp_total_retrans,%d,%d,%d\n",
                    initial->tcp_total_retrans,
                    final->tcp_total_retrans,
                    final->tcp_total_retrans - initial->tcp_total_retrans);
        }

        if (initial->tcp_lost >= 0) {
            fprintf(stdout, "tcp_lost_packets,%d,%d,%d\n",
                    initial->tcp_lost,
                    final->tcp_lost,
                    final->tcp_lost - initial->tcp_lost);
        }

        if (initial->tcp_reordering >= 0) {
            fprintf(stdout, "tcp_reordering,%d,%d,%d\n",
                    initial->tcp_reordering,
                    final->tcp_reordering,
                    final->tcp_reordering - initial->tcp_reordering);
        }

        if (initial->tcp_send_mss > 0) {
            fprintf(stdout, "tcp_send_mss_bytes,%d,%d,%d\n",
                    initial->tcp_send_mss,
                    final->tcp_send_mss,
                    final->tcp_send_mss - initial->tcp_send_mss);
        }

        if (initial->tcp_recv_mss > 0) {
            fprintf(stdout, "tcp_recv_mss_bytes,%d,%d,%d\n",
                    initial->tcp_recv_mss,
                    final->tcp_recv_mss,
                    final->tcp_recv_mss - initial->tcp_recv_mss);
        }

        if (initial->tcp_adv_mss > 0) {
            fprintf(stdout, "tcp_adv_mss_bytes,%d,%d,%d\n",
                    initial->tcp_adv_mss,
                    final->tcp_adv_mss,
                    final->tcp_adv_mss - initial->tcp_adv_mss);
        }

        if (initial->tcp_recv_space > 0) {
            fprintf(stdout, "tcp_recv_space_kb,%d,%d,%d\n",
                    initial->tcp_recv_space / 1024,
                    final->tcp_recv_space / 1024,
                    (final->tcp_recv_space - initial->tcp_recv_space) / 1024);
        }

        return;
    }

    fprintf(stdout, "\n");
    fprintf(stdout,
            "╔═════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout,
            "║                 TCP Metrics Evolution Across All Tests                  ║\n");
    fprintf(stdout,
            "╠═════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stdout,
            "║ %-24s │ %13s │ %13s │ %13s ║\n",
            "Metric", "Initial", "Final", "Total Δ");
    fprintf(stdout,
            "╠═════════════════════════════════════════════════════════════════════════╣\n");

    /* Socket Buffers */
    if (initial->tcp_send_buffer > 0) {
        int delta = final->tcp_send_buffer - initial->tcp_send_buffer;
        fprintf(stdout, "║ %-24s │ %10d KB │ %10d KB │ %9d KB ║\n",
                "TCP Send Buffer",
                initial->tcp_send_buffer / 1024,
                final->tcp_send_buffer / 1024,
                delta / 1024);
    }

    if (initial->tcp_recv_buffer > 0) {
        int delta = final->tcp_recv_buffer - initial->tcp_recv_buffer;
        fprintf(stdout, "║ %-24s │ %10d KB │ %10d KB │ %9d KB ║\n",
                "TCP Recv Buffer",
                initial->tcp_recv_buffer / 1024,
                final->tcp_recv_buffer / 1024,
                delta / 1024);
    }

    /* MSS Values - format as "NUMBER B" right-aligned in 13 chars */
    if (initial->tcp_mss > 0) {
        int delta = final->tcp_mss - initial->tcp_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", initial->tcp_mss);
        snprintf(a, sizeof(a), "%d B", final->tcp_mss);
        snprintf(d, sizeof(d), "%d B", delta);
        fprintf(stdout, "║ %-24s │ %13s │ %13s │ %12s ║\n",
                "TCP MSS", b, a, d);
    }

    /* Congestion Window */
    if (initial->tcp_congestion_window >= 0) {
        int delta = final->tcp_congestion_window - initial->tcp_congestion_window;
        fprintf(stdout, "║ %-24s │ %13d │ %13d │ %12d ║\n",
                "Cwnd (segments)",
                initial->tcp_congestion_window,
                final->tcp_congestion_window,
                delta);
    }

    /* Slow Start Threshold */
    if (initial->tcp_slow_start_threshold >= 0) {
        int delta = final->tcp_slow_start_threshold - initial->tcp_slow_start_threshold;
        fprintf(stdout, "║ %-24s │ %13d │ %13d │ %12d ║\n",
                "SSThresh (segments)",
                initial->tcp_slow_start_threshold,
                final->tcp_slow_start_threshold,
                delta);
    }

    /* RTT */
    if (initial->tcp_rtt > 0) {
        int delta = (int)final->tcp_rtt - (int)initial->tcp_rtt;
        fprintf(stdout, "║ %-24s │ %10.2f ms │ %10.2f ms │ %9.2f ms ║\n",
                "RTT",
                initial->tcp_rtt / 1000.0,
                final->tcp_rtt / 1000.0,
                delta / 1000.0);
    }

    /* RTT Variance */
    if (initial->tcp_rtt_variance > 0) {
        int delta = (int)final->tcp_rtt_variance - (int)initial->tcp_rtt_variance;
        fprintf(stdout, "║ %-24s │ %10.2f ms │ %10.2f ms │ %9.2f ms ║\n",
                "RTT Variance",
                initial->tcp_rtt_variance / 1000.0,
                final->tcp_rtt_variance / 1000.0,
                delta / 1000.0);
    }

    /* Current Retransmissions */
    if (initial->tcp_retrans >= 0) {
        int delta = final->tcp_retrans - initial->tcp_retrans;
        fprintf(stdout, "║ %-24s │ %13d │ %13d │ %12d ║\n",
                "Retrans (current)",
                initial->tcp_retrans,
                final->tcp_retrans,
                delta);
    }

    /* Total Retransmissions */
    if (initial->tcp_total_retrans >= 0) {
        int delta = final->tcp_total_retrans - initial->tcp_total_retrans;
        fprintf(stdout, "║ %-24s │ %13d │ %13d │ %12d ║\n",
                "Total Retrans",
                initial->tcp_total_retrans,
                final->tcp_total_retrans,
                delta);
    }

    /* Lost Packets (if available) */
    if (initial->tcp_lost >= 0) {
        int delta = final->tcp_lost - initial->tcp_lost;
        fprintf(stdout, "║ %-24s │ %13d │ %13d │ %12d ║\n",
                "Lost Packets",
                initial->tcp_lost,
                final->tcp_lost,
                delta);
    }

    /* Reordering (if available) */
    if (initial->tcp_reordering >= 0) {
        int delta = final->tcp_reordering - initial->tcp_reordering;
        fprintf(stdout, "║ %-24s │ %13d │ %13d │ %12d ║\n",
                "Reordering",
                initial->tcp_reordering,
                final->tcp_reordering,
                delta);
    }

    if (initial->tcp_send_mss > 0) {
        int delta = final->tcp_send_mss - initial->tcp_send_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", initial->tcp_send_mss);
        snprintf(a, sizeof(a), "%d B", final->tcp_send_mss);
        snprintf(d, sizeof(d), "%d B", delta);
        fprintf(stdout, "║ %-24s │ %13s │ %13s │ %12s ║\n",
                "TCP Send MSS", b, a, d);
    }

    if (initial->tcp_recv_mss > 0) {
        int delta = final->tcp_recv_mss - initial->tcp_recv_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", initial->tcp_recv_mss);
        snprintf(a, sizeof(a), "%d B", final->tcp_recv_mss);
        snprintf(d, sizeof(d), "%d B", delta);
        fprintf(stdout, "║ %-24s │ %13s │ %13s │ %12s ║\n",
                "TCP Recv MSS", b, a, d);
    }

    if (initial->tcp_adv_mss > 0) {
        int delta = final->tcp_adv_mss - initial->tcp_adv_mss;
        char b[16], a[16], d[16];
        snprintf(b, sizeof(b), "%d B", initial->tcp_adv_mss);
        snprintf(a, sizeof(a), "%d B", final->tcp_adv_mss);
        snprintf(d, sizeof(d), "%d B", delta);
        fprintf(stdout, "║ %-24s │ %13s │ %13s │ %12s ║\n",
                "TCP Adv MSS", b, a, d);
    }

    /* Receive Space (if available) */
    if (initial->tcp_recv_space > 0) {
        int delta = final->tcp_recv_space - initial->tcp_recv_space;
        fprintf(stdout, "║ %-24s │ %10d KB │ %10d KB │ %9d KB ║\n",
                "TCP Recv Space",
                initial->tcp_recv_space / 1024,
                final->tcp_recv_space / 1024,
                delta / 1024);
    }

    fprintf(stdout,
            "╚═════════════════════════════════════════════════════════════════════════╝\n\n");
}

/*!
 * @brief Mark session end - capture "final" metrics and print session summary
*/
int tcp_analytics_session_end(TcpAnalyticsSession *session, CONN *conn)
{
    int ret = tcp_analytics_capture(conn, &session->final);

    if (ret < 0) {
        return ret;
    }

    /* Print session summary */
    print_session_summary(session);
    return 0;
}
