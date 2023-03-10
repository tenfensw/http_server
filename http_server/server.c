//
//  http_server.c
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include <string.h>
#include <unistd.h>
#include "server.h"

//
// private
//
struct sockaddr_in http_make_ipv4(const char* ipAddress,
                                  const http_port_t ipPort) {
    struct sockaddr_in result;
    bzero(&result, sizeof(struct sockaddr_in));
    
    // parse the IP address first
    if (!inet_pton(AF_INET, HI_IF_NULL(ipAddress, HTTP_ADDRESS_PUBLIC), &result.sin_addr)) {
        HI_ERRNO_DEBUG("inet_pton failed, filling ipv4 struct with predefined values");
        
        // default value - listen on any
        result.sin_family = AF_INET;
        result.sin_addr.s_addr = INADDR_ANY;
    }
    
    // adjust port properly
    result.sin_port = htons(ipPort);
    
    return result;
}

struct sockaddr_in6 http_make_ipv6(const char* ipAddress,
                                  const http_port_t ipPort) {
    struct sockaddr_in6 result;
    bzero(&result, sizeof(struct sockaddr_in6));
    
    // parse the IP address first
    // TODO: handle errors correctly
    if (!inet_pton(AF_INET6, HI_IF_NULL(ipAddress, HTTP_ADDRESS_PUBLIC_IPV6), &result.sin6_addr)) {
        HI_ERRNO_DEBUG("inet_pton failed, filling ipv6 struct with predefined values");
        inet_pton(AF_INET6, HTTP_ADDRESS_PUBLIC_IPV6, &result.sin6_addr);
    }
    
    // adjust port properly
    result.sin6_port = htons(ipPort);
    
    return result;
}


void http_getpeerinfo(int sk, char** ipAddressPtr, http_port_t* portPtr) {
    struct sockaddr_in ipv4;
    bzero(&ipv4, sizeof(struct sockaddr_in));
    
    socklen_t length = sizeof(ipv4);
    
    if (getpeername(sk, (struct sockaddr*)&ipv4, &length) == 0) {
        // success
        
        // convert to IPv4
        char* ipAddress = calloc(INET_ADDRSTRLEN, sizeof(char));
        inet_ntop(AF_INET, &ipv4, ipAddress, length);
        
        // obtain port
        http_port_t port = ntohs(ipv4.sin_port);
        
        // save values
        if (ipAddressPtr)
            (*ipAddressPtr) = ipAddress;
        
        if (portPtr)
            (*portPtr) = port;
    } else
        HI_ERRNO_DEBUG("getpeername failed (ipv4)");
}

void http_getpeerinfo6(int sk, char** ipAddressPtr, http_port_t* portPtr) {
    struct sockaddr_in6 ipv6;
    bzero(&ipv6, sizeof(struct sockaddr_in6));
    
    socklen_t length = sizeof(ipv6);
    
    if (getpeername(sk, (struct sockaddr*)&ipv6, &length) == 0) {
        // success
        
        // convert to IPv4
        char* ipAddress = calloc(INET6_ADDRSTRLEN, sizeof(char));
        inet_ntop(AF_INET6, &ipv6, ipAddress, length);
        
        // obtain port
        http_port_t port = ntohs(ipv6.sin6_port);
        
        // save values
        if (ipAddressPtr)
            (*ipAddressPtr) = ipAddress;
        
        if (portPtr)
            (*portPtr) = port;
    } else
        HI_ERRNO_DEBUG("getpeername failed (ipv6)");
}

bool http_server_init_socket(http_server_ref result, bool isIPv6) {
    // init main socket
    result->mainSocket = socket(isIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    
    if (result->mainSocket == 0)
        return false;
    
    // make socket rebindable in case of a crash
    http_size_t tempTrueV = true;
    if (setsockopt(result->mainSocket, SOL_SOCKET, SO_REUSEADDR, &tempTrueV,
                   sizeof(tempTrueV)) != 0) {
        close(result->mainSocket);
        return false;
    }
    
    return true;
}

http_server_ref http_server_init(const char* ipAddress,
                                 const http_port_t ipPort,
                                 const bool useIPv6) {
    http_server_ref result = hizalloc_struct(http_server_s);
    result->clientsMax = HTTP_CLIENTS_MAX;
    
    // init IPv4 socket
    if (!http_server_init_socket(result, false)) {
        HI_ERRNO_DEBUG("init ipv4 socket failed, will return NULL");
        
        // destroy itself on failure
        free(result);
        return NULL;
    }
    
    // import IPv4 listen address
    result->useIPv6 = useIPv6;
    
    if (useIPv6)
        result->ipv6 = http_make_ipv6(ipAddress, ipPort);
    else
        result->ipv4 = http_make_ipv4(ipAddress, ipPort);
    
    // bind when possible
    if (bind(result->mainSocket, (struct sockaddr*)&result->ipv4, sizeof(result->ipv4)) != 0) {
        HI_ERRNO_DEBUG("bind failed, will destroy itself and return NULL");
        
        http_server_release(result);
        return NULL;
    }
    
    // init used in the future multiconnection management via fd_set
    result->clientsFDs = http_fd_set_init(result->clientsMax);
    http_fd_set_set_main_socket(result->clientsFDs, result->mainSocket);
    
    HI_DEBUG("hello world, ipv%u HTTP server initialized, <%p>", useIPv6 ? 6 : 4, result);
    return result;
}

//
// public
//

http_server_ref http_server_init_ipv4(const char* ipAddress,
                                      const http_port_t ipPort) {
    return http_server_init(ipAddress, ipPort, false);
}

http_server_ref http_server_init_ipv6(const char* ipAddress,
                                      const http_port_t ipPort) {
    return http_server_init(ipAddress, ipPort, true);
}

void http_server_set_callback(http_server_ref server,
                              const http_callback_t cb,
                              void* additionalData) {
    if (!server) {
        HI_DEBUG("NULL server instance, no reason to continue");
        return;
    }
    
    // set data & callback
    server->cbData = additionalData;
    server->requestCB = cb;
}

bool http_server_listen(http_server_ref server) {
    if (!server) {
        HI_DEBUG("NULL server parameter specified");
        return false;
    } else if (listen(server->mainSocket, HTTP_PENDING_CONNECTIONS_MAX) != 0) {
        HI_ERRNO_DEBUG("listen failed, returning false");
        return false;
    }
    
    // now that we're listening, roll the event loop
    while (true) {
        // zero and then sync fd_set client descriptors
        http_fd_set_sync_descriptors(server->clientsFDs);
        HI_DEBUG("descriptors synced");
        
        // to handle the new client immediately
        int newClient = 0;
        
        if (http_fd_set_is_set(server->clientsFDs, server->mainSocket)) {
            // new connection
            newClient = accept(server->mainSocket, NULL, NULL);
            HI_DEBUG("new connection %d", newClient);
            
            // add the new client to the array
            if (newClient >= 0)
                http_fd_set_add(server->clientsFDs, newClient, false);
            else {
                // uh oh, error, warn the user
                HI_ERRNO_DEBUG("accept from client failed, here is the reason, will continue as usual");
            }
        }
        
        for (http_size_t sz = 0; sz < server->clientsMax; sz++) {
            int checkedSocket = http_fd_set_get_socket(server->clientsFDs, sz);
            
            // check if there is anything new on the connection front
            if (newClient == checkedSocket || http_fd_set_is_set(server->clientsFDs, checkedSocket)) {
                HI_DEBUG("react to %d", checkedSocket);
                
                char* raw = calloc(HTTP_REQUEST_FIELD_SIZE, sizeof(char));
                ssize_t rawRead = read(checkedSocket, raw, HTTP_REQUEST_FIELD_SIZE);
                
                if (rawRead < 1) {
                    // connection terminated
                    free(raw);
                    
                    HI_DEBUG("client %d saying his goodbyes to us", checkedSocket);
                    http_fd_set_nullify_socket(server->clientsFDs, sz);
                } else {
                    HI_DEBUG("read %d bytes", rawRead);
                    
                    // create request object
                    http_headers_ref request = http_headers_init_with_request(raw, (http_size_t)rawRead);
                    
                    // populate it with IP info
                    char* ipAddress = NULL;
                    http_port_t ipPort = 8080;
                    
                    if (server->useIPv6)
                        http_getpeerinfo6(checkedSocket, &ipAddress, &ipPort);
                    else
                        http_getpeerinfo(checkedSocket, &ipAddress, &ipPort);
                    
                    // save IP info
                    http_headers_set_client_info(request, ipAddress, ipPort);
                    free(ipAddress);
                    
                    HI_DEBUG("headers:");
                    http_headers_debug_dump(request);
                    
                    // prepare for response
                    
                    http_headers_ref response = NULL;
                    char* headersSent = NULL;
                    http_size_t headersSentSize = 0;
                    
                    if (server->requestCB) {
                        response = server->requestCB(request, server->cbData);
                        
                        if (!response) {
                            perror("Callback returned NULL, HTTP server will send 500 Internal Server Error!");
                            perror("If you're the developer of this application, please keep in mind that the HTTP server callback must NEVER return NULL.");
                            
                            // dummy response
                            response = http_headers_init_with_response(500, "text/plain", strdup("error"), 5, free);
                        }
                    } else {
                        const char* staticText = "<h1>Congrats, the server is up!</h1><br> Don't forget to add a callback to handle your own requests.";
                        
                        response = http_headers_init_with_response(200, "text/html", strdup(staticText), (http_size_t)strlen(staticText), free);
                    }
                    
                    headersSent = http_headers_get_response(response, &headersSentSize);
                    
                    // send headers
                    send(checkedSocket, headersSent, headersSentSize, 0);
                    
                    // send body
                    http_size_t respSize = 0;
                    void* resp = http_headers_get_body(response, &respSize);
                    
                    send(checkedSocket, resp, strlen(resp), 0);
                    
                    // goodbye, response and request
                    http_headers_release(response);
                    http_headers_release(request);
                    
                    // TODO: support other responses
                    // TODO: handle properly
                }
            }
        }
    }
}

void http_server_release(http_server_ref server) {
    if (!server)
        return;
    
    // destroy fd_set
    http_fd_set_release(server->clientsFDs);
    // destroy main socket first
    close(server->mainSocket);
    free(server);
}
