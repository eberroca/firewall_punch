#ifndef _UDP_H_
#define _UDP_H_

#include <sys/socket.h>
#include <stdint.h>

typedef struct udp_t {
    uint16_t            port;
    int                 socket;
} udp_t;

int udp_create(int base_port, udp_t **udp);

int udp_destroy(udp_t **udp);

int udp_recv_msg(udp_t *udp, void *buf, size_t len);

int udp_send_msg(udp_t *udp, char *ipv4, int port, void *buf, size_t len);

#endif
