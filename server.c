#include "tcp_conn.h"
#include "proto.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int             port;
    tcp_server_t*   server;
    int             conn;
    int             conn1 = -1;
    int             conn2 = -1;
    proto_t         msg;
    proto_t         msg1;
    proto_t         msg2;

    /** arguments */
    if (argc < 2) {
        fprintf(stderr, "USE: %s <PORT>\n", argv[0]);
        return -1;
    }
    port = atoi(argv[1]);

    if (tcp_conn_create_server(port, 5, &server) < 0) {
        return -1;
    }

    fprintf(stdout, "server created\n");

    /** accepting new connections */
    while (1) {
        if (tcp_conn_new_connection(server, &conn) < 0) {
            return -1;
        }
        
        fprintf(stdout, "new connection from %s\n",
                server->active_conns[conn].ipv4);

        if (tcp_conn_recv_msg(server,
                              conn,
                              (void *)&msg,
                              sizeof(proto_t)) < 0) {
            return -1;
        }

        fprintf(stdout, "received command %d\n", msg.cmd);

        if (msg.cmd == 1) { // sender
            if (conn1 >= 0) {
                fprintf(stdout, "recevied command 1 twice! destroying ");
                fprintf(stdout, "connection...\n");

                if (tcp_conn_destroy_connection(server, conn) < 0) {
                    return -1;
                }
                continue;
            }
            conn1 = conn;
            msg1  = msg;
            strcpy(msg1.ipv4, server->active_conns[conn].ipv4);

        } else if (msg.cmd == 2) { // receiver
            if (conn2 >= 0) {
                fprintf(stdout, "recevied command 2 twice! destroying ");
                fprintf(stdout, "connection...\n");
                
                if (tcp_conn_destroy_connection(server, conn) < 0) {
                    return -1;
                }
                continue;
            }
            conn2 = conn;
            msg2  = msg;
            strcpy(msg2.ipv4, server->active_conns[conn].ipv4);
        }

        if (conn1 >=0 && conn2 >= 0) {
            /** send sender info to receiver */
            if (tcp_conn_send_msg(server,
                                  conn2,
                                  (void *)&msg1,
                                  sizeof(proto_t)) < 0) {
                return -1;
            }

            /** wait for receiver permission for transfer start */
            if (tcp_conn_recv_msg(server,
                                  conn2,
                                  (void *)&msg,
                                  sizeof(proto_t)) < 0) {
                return -1;
            }

            /** send the receiver info to sender */
            if (tcp_conn_send_msg(server,
                                  conn1,
                                  (void *)&msg2,
                                  sizeof(proto_t)) < 0) {
                return -1;
            }

            /** destroy connections */
            if (tcp_conn_destroy_connection(server, conn1) < 0) {
                return -1;
            }
            if (tcp_conn_destroy_connection(server, conn2) < 0) {
                return -1;
            }
            conn1 = -1;
            conn2 = -1;
        }
    }
    // we never really reach this part of the code
    return 0;
}
