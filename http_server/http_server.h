//
//  http_server.h
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

//
// constant values
//

/// default max clients value for the HTTP server
#define HTTP_CLIENTS_MAX 30
/// default pending connections limit for the HTTP server
#define HTTP_PENDING_CONNECTIONS_MAX 1
/// default request field size limit for the HTTP server
#define HTTP_REQUEST_FIELD_SIZE 4096

//
// default constant IP address values (IPv4)
//
#define HTTP_ADDRESS_LOCALHOST "127.0.0.1"
#define HTTP_ADDRESS_PUBLIC "0.0.0.0"

//
// primitive types
//

/// universal index value for libhttpserver
typedef uint32_t http_size_t;

/// HTTP server port value
typedef uint16_t http_port_t;

//
// complex types
//

/// internally-used fd_set wrapper managing multiple client connections
typedef struct http_fd_set_s* http_fd_set_ref;

/// HTTP headers map object
typedef struct http_headers_s* http_headers_ref;

/// HTTP server control object
typedef struct http_server_s* http_server_ref;

//
// http_server_ref: HTTP server methods
//

http_server_ref http_server_init_ipv4(const char* ipAddress,
                                      const http_port_t ipPort);

bool http_server_listen(http_server_ref server);

void http_server_release(http_server_ref server);
