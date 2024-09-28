#include "tcp_conn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int tcp_conn_create_client(tcp_client_t **client)
{
    if ((*client = (tcp_client_t *)calloc(1, sizeof(tcp_client_t))) == NULL) {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        return -1;
    }

    if (((*client)->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error creating TCP socket: %s\n", strerror(errno));
        free(*client);
        *client = NULL;
        return -1;
    }

    return 0;
}

int tcp_conn_destroy_client(tcp_client_t **client)
{
    int ret = 0;

    if (close((*client)->socket)) {
        fprintf(stderr, "Error closing TCP socket: %s\n", strerror(errno));
        ret = -1;
    }

    free(*client);
    *client = NULL;
    return ret;
}

int tcp_conn_connecto_server(tcp_client_t *client, char *ipv4, int port)
{
    struct sockaddr_in  server_addr;

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ipv4);
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (connect(client->socket,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) != 0) {
        fprintf(stderr, "Error connecting TCP socket: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int tcp_conn_client_send_msg(tcp_client_t *client, void *buf, size_t len)
{
    size_t  rc;

    rc = send(client->socket, buf, len, 0);
    if (rc < 0) {
        fprintf(stderr, "Error sending TCP packages: %s\n", strerror(errno));
        return -1;
    }
    if (rc < len) {
        fprintf(stderr, "Error sending TCP packages: rc=%zu < len=%zd\n",
                rc,
                len);
        return -1;
    }

    return 0;
}

int tcp_conn_client_recv_msg(tcp_client_t *client, void *buf, size_t len)
{
    ssize_t rc;
    size_t  total = 0;
    char*   buf_ptr = (char *)buf;

    while (total < len) {
        rc = recv(client->socket, &buf_ptr[total], len - total, 0);
        if (rc < 0) {
            fprintf(stderr, "Error receiving TCP package: %s\n",
                    strerror(errno));
            return -1;
        }
        total += rc;
    }

    return 0;
}

int tcp_conn_create_server(int port, int max_conns, tcp_server_t **server)
{
    struct sockaddr_in  server_addr;

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Incorrect port number: port=%d\n", port);
        return -1;
    }
    if (max_conns <= 0) {
        fprintf(stderr, "Incorrect number of max connections: max_conns=%d\n",
                max_conns);
        return -1;
    }

    /** creating struct */
    if ((*server = (tcp_server_t *)calloc(1, sizeof(tcp_server_t))) == NULL) {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        return -1;
    }
    (*server)->max_conns = max_conns;

    /** creating list of active connections */
    if (((*server)->active_conns = (tcp_server_conn_t *)
                    calloc(max_conns, sizeof(tcp_server_conn_t))) == NULL) {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        goto error0;
    }

    /** creating dict of active connections */
    if (((*server)->conns_dict = (uint8_t *) calloc(max_conns,
                                                    sizeof(uint8_t))) == NULL) {
        fprintf(stderr, "Error allocating memory: %s\n", strerror(errno));
        goto error1;
    }

    /** creating TCP socket */
    (*server)->port = (uint16_t)port;
    if (((*server)->listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error creating TCP socket: %s\n", strerror(errno));
        goto error2;
    }

    /** configure server addr struct */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    // for fix IP ->  "= inet_addr("127.0.0.1");
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    /** bind addr struct to TCP socket */
    if (bind((*server)->listening_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) != 0) {
        fprintf(stderr, "Error binding address to TCP socket: %s\n",
                strerror(errno));
        goto error2;
    }

    /** listening for new connections */
    if (listen((*server)->listening_socket, max_conns) != 0) {
        fprintf(stderr, "Error listening on TCP socket: %s\n",
                strerror(errno));
        goto error2;
    }

    return 0;
error2:
    free((*server)->conns_dict);
error1:
    free((*server)->active_conns);
error0:
    free(*server);
    *server = NULL;
    return -1;
}

int tcp_conn_destroy_server(tcp_server_t **server)
{
    int ret = 0;
    int i;
    
    for (i = 0; i < (*server)->max_conns; i++) {
        if ((*server)->conns_dict[i]) {
            ret = tcp_conn_destroy_connection(*server, i);
        }
    }

    if (close((*server)->listening_socket)) {
        fprintf(stderr, "Error closing listening TCP socket: %s\n",
                strerror(errno));
        ret = -1;
    }

    free((*server)->conns_dict);
    free((*server)->active_conns);
    free(*server);
    *server = NULL;
exit:
    return ret;
}

int tcp_conn_new_connection(tcp_server_t *server, int *conn)
{
    int                 new_socket;
    struct sockaddr_in* addr_in;
    int                 i;
    tcp_server_conn_t*  connection;

    *conn = -1;

    server->server_storage_len = sizeof(server->server_storage);
    new_socket = accept(server->listening_socket,
                        (struct sockaddr *)&server->server_storage,
                        &server->server_storage_len);
    if (new_socket < 0) {
        fprintf(stderr, "Error accepting TCP connection: %s\n",
                strerror(errno));
        return -1;
    }

    for (i = 0; i < server->max_conns; i++) {
        if (0 == server->conns_dict[i]) {
            *conn = i;
            server->conns_dict[i] = 1;
            break;
        }
    }
    if (-1 == *conn) {
        fprintf(stderr, "Error reached max number of connections!\n");
        return -1;
    }

    connection = &server->active_conns[*conn];

    connection->socket = new_socket;
    addr_in            = (struct sockaddr_in *)&server->server_storage;
    strcpy(connection->ipv4, inet_ntoa(addr_in->sin_addr));

    return 0;
}

int tcp_conn_destroy_connection(tcp_server_t *server, int conn)
{
    tcp_server_conn_t*  connection;

    if (conn < 0 || conn >= server->max_conns) {
        fprintf(stderr, "Incorrect connection number %d\n", conn);
        return -1;
    }

    if (0 == server->conns_dict[conn]) {
        fprintf(stdout,
                "Warning: trying to destroy a non-existing connection\n");
        return 0;
    }
    server->conns_dict[conn] = 0;

    connection = &server->active_conns[conn];

    if (close(connection->socket) < 0) {
        fprintf(stderr, "Error closing TCP connection: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int tcp_conn_recv_msg(tcp_server_t *server, int conn, void *buf, size_t len)
{
    tcp_server_conn_t*  connection;
    int                 error = 0;
    ssize_t             rc;
    size_t              total = 0;
    char*               buf_ptr = (char *)buf;

    if (conn < 0 || conn > server->max_conns) {
        error = 1;
    }
    if (error || 0 == server->conns_dict[conn]) {
        fprintf(stderr, "Incorrect connection number %d\n", conn);
        return -1;
    }

    connection = &server->active_conns[conn];

    while (total < len) {
        rc = recv(connection->socket, &buf_ptr[total], len - total, 0);
        if (rc < 0) {
            fprintf(stderr, "Error receiving TCP package: %s\n",
                    strerror(errno));
            return -1;
        }
        total += rc;
    }

    return 0;
}

int tcp_conn_send_msg(tcp_server_t *server, int conn, void *buf, size_t len)
{
    tcp_server_conn_t*  connection;
    int                 error = 0;
    size_t              rc;

    if (conn < 0 || conn > server->max_conns) {
        error = 1;
    }
    if (error || 0 == server->conns_dict[conn]) {
        fprintf(stderr, "Incorrect connection number %d\n", conn);
        return -1;
    }

    connection = &server->active_conns[conn];

    rc = send(connection->socket, buf, len, 0);
    if (rc < 0) {
        fprintf(stderr, "Error sending TCP packages: %s\n", strerror(errno));
        return -1;
    }
    if (rc < len) {
        fprintf(stderr, "Error sending TCP packages: rc=%zu < len=%zd\n",
                rc,
                len);
        return -1;
    }

    return 0;
}

