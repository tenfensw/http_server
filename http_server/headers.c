//
//  headers.c
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include "headers.h"

//
// private
//

struct http_pair_s {
    char* key;
    char* value;
    
    http_pair_ref next;
};

http_pair_ref http_pair_find_by_key(http_pair_ref first, const char* key,
                                    http_size_t* countPtr) {
    http_pair_ref current = first;
    http_size_t count = 1;
    
    while (current) {
        if (key) {
            if (strcmp(current->key, key) == 0)
                return current;
        }
        
        current = current->next;
        count++;
    }
    
    if (countPtr)
        (*countPtr) = count;
    
    return NULL;
}

http_pair_ref http_pair_init(const char* key, const char* value) {
    http_pair_ref pair = hizalloc_struct(http_pair_s);
    
    pair->key = strdup(key);
    pair->value = strdup(value);
    
    return pair;
}

void http_pair_chain_release(http_pair_ref pair) {
    http_pair_ref current = pair;
    
    while (current) {
        http_pair_ref next = current->next;
        http_pair_release(current);
        
        current = next;
    }
}

void http_pair_release(http_pair_ref pair) {
    if (!pair)
        return;
    
    free(pair->key);
    free(pair->value);
    free(pair);
}

bool http_headers_parse_request(http_headers_ref headers,
                                const char* raw,
                                const http_size_t rawSize) {
    if (!raw || rawSize < 1) {
        HI_DEBUG("raw request headers empty, cannot continue");
        return false;
    }
    
    // state machine
    http_request_parse_state_t state = HTTP_PARSE_STATE_METHOD;
    
    // token for the first few parts
    char* token = calloc(HTTP_REQUEST_URL_LENGTH, sizeof(char));
    http_size_t tokenC = 0;
    
    for (http_size_t sz = 0; sz < rawSize; sz++) {
        char current = raw[sz];
        HI_DEBUG("current = '%c'", current);
        
        // skip it, it's required by standard but is basically used to support
        // \r\n platformd
        if (current == '\r')
            continue;
        
        if (state < HTTP_PARSE_STATE_PAIR) {
            if (isspace(current) == 0 && tokenC < HTTP_REQUEST_URL_LENGTH)
                token[tokenC++] = current;
            else {
                switch (state) {
                    case HTTP_PARSE_STATE_METHOD: {
                        headers->requestType = strdup(token);
                        break;
                    }
                    case HTTP_PARSE_STATE_URI: {
                        headers->requestURL = strdup(token);
                        break;
                    }
                    case HTTP_PARSE_STATE_VERSION: {
                        headers->requestVersion = strdup(token);
                        break;
                    }
                    default:
                        break;
                }
                
                HI_DEBUG("token = \"%s\", state = %u", token, state);
                
                // clear token and move on
                bzero(token, HTTP_REQUEST_URL_LENGTH);
                tokenC = 0;
                state++;
                
                if (state > HTTP_PARSE_STATE_VERSION)
                    free(token); // not needed anymore
                
            }
        } else {
            // header key-value pair
            char* key = calloc(HTTP_HEADER_LENGTH_MAX, sizeof(char));
            http_size_t keySize = 0;
            
            bool endOfHeaders = false;
            
            while (keySize < HTTP_HEADER_LENGTH_MAX) {
                if (current == '\n') {
                    // end of headers found
                    endOfHeaders = true;
                    break;
                } else if (current == '\r')
                    continue;
                
                current = raw[sz++];
                
                if (current != ':')
                    key[keySize++] = current;
                else
                    break;
            }
            
            if (endOfHeaders) {
                free(key);
               
                // if GET, then nothing left to do
                // TODO: HEAD, DELETE, OPTIONS also needs nothing in body
                if (strcmp(headers->requestType, "GET") != 0) {
                    sz++;
                    
                    // read the body
                    http_size_t leftToRead = rawSize - sz;
                    const char* rawCL = http_headers_get(headers, "Content-Length");
                    
                    // if specified, use Content-Length
                    if (rawCL)
                        leftToRead = (http_size_t)atoi(rawCL);
                    
                    char* body = calloc(leftToRead, sizeof(char));
                    memcpy(body, raw + sz, leftToRead);
                    
                    // save read body into headers
                    headers->body = body;
                    headers->bodyDLC = (http_deallocator_t)free;
                }
                
                break;
            }
            
            // read in value now
            char* value = calloc(HTTP_REQUEST_URL_LENGTH, sizeof(char));
            http_size_t valueSize = 0;
            
            while (valueSize < HTTP_REQUEST_URL_LENGTH) {
                current = raw[sz++];
                
                if (isspace(current) != 0 && valueSize < 1)
                    continue;
                else if (current != '\r' && current != '\n')
                    value[valueSize++] = current;
                else
                    break;
            }
            
            HI_DEBUG("read header \"%s\" (%u) with value (%u): |%s|", key, keySize,
                     valueSize, value);
            
            http_headers_set(headers, key, value);
            
            // remove unnecessary items
            free(key);
            free(value);
        }
    }
    
    return true;
}

//
// public
//

http_headers_ref http_headers_init_with_request(const char* raw,
                                                const http_size_t rawSize) {
    http_headers_ref headers = hizalloc_struct(http_headers_s);
    
    // parse request headers
    if (!http_headers_parse_request(headers, raw, rawSize))
        HI_DEBUG("failed to parse headers properly, will move on as is");
    
    return headers;
}

http_headers_ref http_headers_init_with_response(const http_status_t status,
                                                 const char* contentType,
                                                 void* body,
                                                 const http_size_t bodySize,
                                                 const http_deallocator_t bodyDLC) {
    http_headers_ref headers = hizalloc_struct(http_headers_s);
    
    // set the appropriate headers
    http_headers_set(headers, "Content-Type", HI_IF_NULL(contentType, "application/octet-stream"));
    http_headers_set_int(headers, "Content-Length", bodySize);
    
    // set body and status
    headers->body = body;
    headers->bodyDLC = bodyDLC;
    
    headers->statusCode = status;
    return headers;
}

const char* http_headers_get(const http_headers_ref headers,
                             const char* key) {
    if (!headers || !key)
        return NULL;
    
    http_pair_ref result = http_pair_find_by_key(headers->first, key, NULL);
    return (result ? result->value : NULL);
}

void http_headers_debug_dump(http_headers_ref headers) {
    if (!headers)
        HI_DEBUG("(null)");
    else {
        if (headers->requestType && headers->requestURL)
            HI_DEBUG("%s %s %s", headers->requestType, headers->requestURL,
                     HI_IF_NULL(headers->requestVersion, "HTTP/1.0"));
        
        http_pair_ref current = headers->first;
        
        while (current) {
            HI_DEBUG("%s: %s", current->key, current->value);
            current = current->next;
        }
        
        if (headers->ipAddress)
            HI_DEBUG("requested from %s with port %u", headers->ipAddress,
                     headers->port);
        
        if (headers->body)
            HI_DEBUG("body:\n%s", (char*)headers->body);
    }
}

bool http_headers_set(http_headers_ref headers,
                      const char* key,
                      const char* value) {
    if (!headers || !key || strlen(key) < 1 || !value) {
        HI_DEBUG("self <%p>, key <%p> or value <%p> invalid, not doing anything");
        return false;
    }
    
    http_pair_ref last = http_pair_find_by_key(headers->first, key, NULL);
    if (last) {
        // replace value
        free(last->value);
        last->value = strdup(value);
    } else if (!headers->first)
        headers->first = http_pair_init(key, value);
    else {
        // insert self as second
        http_pair_ref newNext = headers->first->next;
        headers->first->next = http_pair_init(key, value);
        headers->first->next->next = newNext;
    }
    
    return true;
}

bool http_headers_set_int(http_headers_ref headers,
                          const char* key,
                          const http_ssize_t value) {
    // convert to int and set the value
    char* valueStr = hiitoa(value);
    bool result = http_headers_set(headers, key, valueStr);
    
    return result;
}

void http_headers_set_client_info(http_headers_ref headers,
                                  const char* ipAddress,
                                  const http_port_t ipPort) {
    if (!headers)
        return;
    
    // clean up in advance
    free(headers->ipAddress);
    headers->port = 0;
    
    // set IP if necessary
    if (ipAddress) {
        headers->ipAddress = strdup(ipAddress);
        headers->port = ipPort;
    }
}

char* http_headers_get_response(const http_headers_ref headers,
                                http_size_t* sizePtr) {
    if (!headers) {
        HI_DEBUG("NULL headers, cannot generate reponse");
        return NULL;
    }
    
    char* heading = calloc(HTTP_HEADER_LENGTH_MAX, sizeof(char));
    
    // first the main entry (TODO: proper status code strings)
    sprintf(heading, "%s %u OK\r\n", HI_IF_NULL(headers->requestVersion, "HTTP/1.1"),
            headers->statusCode);
    
    http_size_t length = (http_size_t)strlen(heading);
    
    // add each header to the heading
    http_pair_ref current = headers->first;
    
    while (current) {
        strcat(heading, current->key);
        strcat(heading, ": ");
        strcat(heading, current->value);
        strcat(heading, "\r\n");
        
        length += strlen(current->key) + 4 + strlen(current->value);
        current = current->next;
    }
    
    strcat(heading, "\r\n");
    length += 2;
   
    // save size
    if (sizePtr)
        (*sizePtr) = length;
    
    return heading;
}

void* http_headers_get_body(const http_headers_ref headers,
                            http_size_t* sizePtr) {
    // TODO: optimize
    if (!headers || !headers->body)
        return NULL;
    
    // set size
    if (sizePtr)
        (*sizePtr) = atoi(HI_IF_NULL(http_headers_get(headers, "Content-Length"), "0"));
    
    return headers->body;
}

void http_headers_release(http_headers_ref headers) {
    if (!headers)
        return;
    
    // delete all keys first
    http_pair_chain_release(headers->first);
    headers->first = NULL;
    
    // free all strings
    free(headers->ipAddress);
    free(headers->requestType);
    free(headers->requestURL);
    free(headers->requestVersion);
    
    // deallocate raw body if necessary
    if (headers->bodyDLC)
        headers->bodyDLC(headers->body);
    
    free(headers);
}
