//
//  server.h
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include "fds.h"

struct http_server_s {
    // IP address to listen on
    union {
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    };
    // if true, use server->ipv6 field for valid address
    bool useIPv6;
    
    // maximum accepted connections at a time
    http_size_t clientsMax;
    // client connections managed by a fd_set wrapper
    http_fd_set_ref clientsFDs;
    
    // main listening socket
    int mainSocket;
    
    // callback called on every request
    http_callback_t requestCB;
    // custom dev data passed to every callback call
    void* cbData;
};

struct sockaddr_in http_make_ipv4(const char* ipAddress,
                                  const http_port_t ipPort);

void http_getpeerinfo(int sk, char** ipAddressPtr, http_port_t* portPtr);
void http_getpeerinfo6(int sk, char** ipAddressPtr, http_port_t* portPtr);

bool http_server_init_socket(http_server_ref result, bool isIPv6);
