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
/// default acceptable URL length size
#define HTTP_REQUEST_URL_LENGTH 2048

#define HTTP_HEADER_LENGTH_MAX 128

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
typedef int32_t http_ssize_t;

/// deallocator callback type for http_headers_ref raw data
/// memory management
typedef void (*http_deallocator_t)(void*);

/// HTTP server port value
typedef uint16_t http_port_t;

/// HTTP server status codes
typedef enum {
    HTTP_OK = 200,
    
    // 200s - success
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    
    // 300s - redirects
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_FOUND = 302,
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_PERMANENT_REDIRECT = 308,
    
    // 400s - client errors
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    
    // 410s - processing issues
    HTTP_PAYLOAD_TOO_LARGE = 413,
    HTTP_URL_TOO_LONG = 414,
    
    // 500s - server errors
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_UNSUPPORTED_VERSION = 505,
    
    // misc
    HTTP_TEAPOT = 418,
    HTTP_RESERVED = 420
} http_status_t;

//
// complex types
//

/// internally-used fd_set wrapper managing multiple client connections
typedef struct http_fd_set_s* http_fd_set_ref;

/// HTTP headers map & universal request/response object
typedef struct http_headers_s* http_headers_ref;

/// HTTP server control object
typedef struct http_server_s* http_server_ref;

/// HTTP server central route callback
typedef http_headers_ref (*http_callback_t)(const http_headers_ref,
                                            void*);

//
// http_headers_ref: HTTP headers methods
//
http_headers_ref http_headers_init_with_request(const char* raw,
                                                const http_size_t rawSize);
http_headers_ref http_headers_init_with_response(const http_status_t status,
                                                 const char* contentType,
                                                 void* body,
                                                 const http_size_t bodySize,
                                                 const http_deallocator_t bodyDLC);
http_headers_ref http_headers_init_empty(void);

const char* http_headers_get(const http_headers_ref headers,
                             const char* key);

bool http_headers_set(http_headers_ref headers,
                      const char* key,
                      const char* value);
bool http_headers_set_int(http_headers_ref headers,
                          const char* key,
                          const http_ssize_t value);

void http_headers_set_client_info(http_headers_ref headers,
                                  const char* ipAddress,
                                  const http_port_t ipPort);

/// get appropriately formatted HTTP/1.1 response headers
char* http_headers_get_response(const http_headers_ref headers,
                                http_size_t* sizePtr);

void* http_headers_get_body(const http_headers_ref headers,
                            http_size_t* sizePtr);

void http_headers_debug_dump(http_headers_ref headers);

void http_headers_release(http_headers_ref headers);

//
// http_server_ref: HTTP server methods
//

http_server_ref http_server_init_ipv4(const char* ipAddress,
                                      const http_port_t ipPort);

void http_server_set_callback(http_server_ref server,
                              const http_callback_t cb,
                              void* additionalData);

bool http_server_listen(http_server_ref server);

void http_server_release(http_server_ref server);


