#ifndef _PROTO_H_
#define _PROTO_H_

typedef struct proto_t {
    int     cmd;
    int     port;
    char    ipv4[16];
    char    msg[32];
} proto_t;

#endif
