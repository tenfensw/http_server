//
//  headers.h
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#pragma once

#include "wrappers.h"

/// internally-used key-value storing object
typedef struct http_pair_s* http_pair_ref;

struct http_headers_s {
    // first header reference
    http_pair_ref first;
    
    // HTTP status code
    http_status_t statusCode;
    
    // client-supported HTTP version
    char* requestVersion;
    // requested URI
    char* requestURL;
    // client request type
    char* requestType; // "GET", "POST", etc
    
    // raw body contents
    void* body;
    // raw body deallocator
    http_deallocator_t bodyDLC;
    
    // client IP address
    char* ipAddress;
    // client port
    http_port_t port;
};

typedef enum {
    // GET, POST, misc
    HTTP_PARSE_STATE_METHOD = 0,
    // URI
    HTTP_PARSE_STATE_URI,
    // HTTP version
    HTTP_PARSE_STATE_VERSION,
    
    // header key-pair
    HTTP_PARSE_STATE_PAIR
} http_request_parse_state_t;

bool http_headers_parse_request(http_headers_ref headers,
                                const char* raw,
                                const http_size_t rawSize);

//
// pair-related
//

http_pair_ref http_pair_init(const char* key, const char* value);

http_pair_ref http_pair_find_by_key(http_pair_ref first, const char* key,
                                    http_size_t* countPtr);

void http_pair_chain_release(http_pair_ref pair);
void http_pair_release(http_pair_ref pair);
