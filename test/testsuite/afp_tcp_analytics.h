#ifndef AFP_TCP_ANALYTICS_H
#define AFP_TCP_ANALYTICS_H

#include <stdint.h>
#include <sys/types.h>  /* for off_t */
#include "afpclient.h"  /* for CONN */

/* Output format enumeration */
typedef enum {
    TCP_OUTPUT_TEXT,
    TCP_OUTPUT_CSV
} TcpOutputFormat;

/* TCP metrics structure */
typedef struct {
    uint32_t server_quantum;
    int tcp_send_buffer;
    int tcp_recv_buffer;
    int tcp_send_window;
    int tcp_mss;
    int tcp_congestion_window;
    int tcp_slow_start_threshold;
    uint32_t tcp_rtt;
    uint32_t tcp_rtt_variance;
    int tcp_retrans;
    int tcp_total_retrans;
    int tcp_reordering;
    int tcp_lost;
    int tcp_send_mss;
    int tcp_recv_mss;
    int tcp_adv_mss;
    int tcp_recv_space;
    int is_localhost;
} TcpMetrics;

/* Session handle for tracking metrics across multiple tests */
typedef struct {
    TcpMetrics initial;              /* Captured at session start */
    TcpMetrics before;               /* Captured before current test */
    TcpMetrics after;                /* Captured after current test */
    TcpMetrics final;                /* Captured at session end */
    off_t current_size;              /* Current test file size */
    TcpOutputFormat format;          /* TEXT or CSV */
    int enable_session_tracking;     /* For size sweep mode */
} TcpAnalyticsSession;

/* API Functions */
void tcp_analytics_init(TcpAnalyticsSession *session, TcpOutputFormat format,
                        int enable_session);
int tcp_analytics_capture(CONN *conn, TcpMetrics *metrics);
int tcp_analytics_session_start(TcpAnalyticsSession *session, CONN *conn);
int tcp_analytics_test_start(TcpAnalyticsSession *session, CONN *conn,
                             off_t file_size);
int tcp_analytics_test_end(TcpAnalyticsSession *session, CONN *conn);
int tcp_analytics_session_end(TcpAnalyticsSession *session, CONN *conn);

#endif /* AFP_TCP_ANALYTICS_H */
