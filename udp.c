#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int udp_create(int base_port, udp_t **udp)
{
    int                 rc;
    struct sockaddr_in  addr;

    if (base_port <= 0 || base_port > 65535) {
        fprintf(stderr, "Incorrect port number: port=%d\n", base_port);
        return -1;
    }

    /** creating UDP struct */
    if ((*udp = (udp_t *)calloc(1, sizeof(udp_t))) == NULL) {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        return -1;
    }
    (*udp)->port = base_port;

    /** create UDP socket */
    if (((*udp)->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Error creating UDP socket: %s\n", strerror(errno));
        goto error;
    }

    /** bind to UDP socket */
    addr.sin_family         = AF_INET;
    addr.sin_port           = htons((*udp)->port);
    addr.sin_addr.s_addr    = INADDR_ANY;
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    while (1) {
        rc = bind((*udp)->socket, (struct sockaddr *)&addr, sizeof(addr));
        if (0 == rc) {
            break;
        }
        if (errno == EADDRINUSE && (*udp)->port < 65535) {
            (*udp)->port    += 1;
            addr.sin_port   = htons((*udp)->port);
            continue;
        }
        fprintf(stderr, "Error binding address to UDP socket: %s\n",
                strerror(errno));
        goto error;
    }

    return 0;
error:
    free(*udp);
    *udp = NULL;
    return -1;
}

int udp_destroy(udp_t **udp)
{
    int ret = 0;

    if (close((*udp)->socket)) {
        fprintf(stderr, "Error closing UDP socket: %s\n", strerror(errno));
        ret = -1;
    }

    free(*udp);
    *udp = NULL;
    return ret;
}

int udp_recv_msg(udp_t *udp, void *buf, size_t len)
{
    ssize_t rc;
    size_t  total = 0;
    char*   buf_ptr = (char *)buf;

    while (total < len) {
        rc = recv(udp->socket, &buf_ptr[total], len - total, 0);
        if (rc < 0) {
            fprintf(stderr, "Error receiving UDP package: %s\n",
                    strerror(errno));
            return -1;
        }
        total += rc;
    }
    
    return 0;
}

int udp_send_msg(udp_t *udp, char *ipv4, int port, void *buf, size_t len)
{
    ssize_t             rc;
    struct sockaddr_in  addr;

    addr.sin_family         = AF_INET;
    addr.sin_port           = htons(port);
    addr.sin_addr.s_addr    = inet_addr(ipv4);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    rc = sendto(udp->socket,
                buf,
                len,
                0,
                (struct sockaddr *)&addr,
                sizeof(addr));
    if (rc < 0) {
        fprintf(stderr, "Error sending UDP msg: %s\n", strerror(errno));
        return -1;
    }
    if (rc < len) {
        fprintf(stderr, "Error sending UDP msg: rc=%zu < len=%zd\n", rc, len);
        return -1;
    }

    return 0;
}
