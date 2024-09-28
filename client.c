#include "tcp_conn.h"
#include "udp.h"
#include "proto.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
    char*           serverIP;
    int             serverPort;
    int             cmd;
    tcp_client_t*   tcp_client;
    udp_t*          udp;
    proto_t         msg;
    proto_t         msg1;

    if (argc < 4)
    {
        fprintf(stderr, "USE: %s <SERVER_IP_ADDR> <SERVER_PORT> <CMD>\n",
            argv[0]);
        return -1;
    }
    serverIP   = argv[1];
    serverPort = (uint16_t) atoi(argv[2]);
    cmd        = (int) atoi(argv[3]);

    if (cmd < 1 || cmd > 2) {
        fprintf(stderr, "Error, wrong command -> %d\n", cmd);
        return -1;
    }

    if (tcp_conn_create_client(&tcp_client) < 0) {
        return -1;
    }

    if (udp_create(1234, &udp) < 0) {
        return -1;
    }

    if (tcp_conn_connecto_server(tcp_client, serverIP, serverPort) < 0) {
        return -1;
    }

    /** send data to server with command and UDP port */
    msg.cmd   = cmd;
    msg.port  = udp->port;
    if (tcp_conn_client_send_msg(tcp_client, (void *)&msg, sizeof(msg)) < 0) {
        return -1;
    }

    /** receive data from server again (info from other client) */
    if (tcp_conn_client_recv_msg(tcp_client,
                                 (void *)&msg1,
                                 sizeof(msg1)) < 0) {
        return -1;
    }

    if (cmd == 1) {
    
        /** Send UDP package through firewall hole !! */
        strcpy(msg.msg, "Hello World!");

        fprintf(stdout, "sending UDP with message = %s\n", msg.msg);

        if (udp_send_msg(udp,
                         msg1.ipv4,
                         msg1.port,
                         (void *)&msg,
                         sizeof(msg)) < 0) {
            return -1;
        }
    } else { /* cmd == 2 */

        /** create firewall hole */
        if (udp_send_msg(udp,
                         msg1.ipv4,
                         msg1.port,
                         (void *)&msg,
                         sizeof(msg)) < 0) {
            return -1;
        }

        /** Let server know that the hole was punched */
        if (tcp_conn_client_send_msg(tcp_client,
                                     (void *)&msg,
                                     sizeof(msg)) < 0) {

        }

        /** receive UDP package */
        if (udp_recv_msg(udp, (void *)&msg1, sizeof(msg1)) < 0) {
            return -1;
        }

        fprintf(stdout, "(UDP) received !!!!\n");
        fprintf(stdout, "message = %s\n", msg1.msg);
    }

    if (tcp_conn_destroy_client(&tcp_client) < 0) {
        return -1;
    }

    if (udp_destroy(&udp) < 0) {
        return -1;
    }

    return 0;
}
