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
};

struct sockaddr_in http_make_ipv4(const char* ipAddress,
                                  const http_port_t ipPort);

bool http_server_init_socket(http_server_ref result, bool isIPv6);
