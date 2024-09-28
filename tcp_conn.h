#ifndef _TCP_CONN_H_
#define _TCP_CONN_H_

#include <sys/socket.h>
#include <stdint.h>

typedef struct tcp_server_conn_t {
    int     socket;
    char    ipv4[16];
} tcp_server_conn_t;

typedef struct tcp_server_t {
    uint16_t                port;
    int                     listening_socket;
    struct sockaddr_storage server_storage;
    socklen_t               server_storage_len;
    int                     max_conns;
    tcp_server_conn_t*      active_conns;
    uint8_t*                conns_dict;
} tcp_server_t;

typedef struct tcp_client_t {
    int socket;
} tcp_client_t;

int tcp_conn_create_client(tcp_client_t **client);

int tcp_conn_destroy_client(tcp_client_t **client);

int tcp_conn_connecto_server(tcp_client_t *client, char *ipv4, int port);

int tcp_conn_client_send_msg(tcp_client_t *client, void *buf, size_t len);

int tcp_conn_client_recv_msg(tcp_client_t *client, void *buf, size_t len);

int tcp_conn_create_server(int port, int max_conns, tcp_server_t **server);

int tcp_conn_destroy_server(tcp_server_t **server);

int tcp_conn_new_connection(tcp_server_t *server, int *conn);

int tcp_conn_destroy_connection(tcp_server_t *server, int conn);

int tcp_conn_recv_msg(tcp_server_t *server, int conn, void *buf, size_t len);

int tcp_conn_send_msg(tcp_server_t *server, int conn, void *buf, size_t len);

#endif
