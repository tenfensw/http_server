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

//
// public
//

http_server_ref http_server_init_ipv4(const char* ipAddress,
                                      const http_port_t ipPort) {
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
    result->useIPv6 = false;
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
    
    HI_DEBUG("hello world, ipv4 HTTP server initialized, <%p>", result);
    return result;
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
        HI_DEBUG("descriptors syned");
        
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
                    HI_DEBUG("read %d bytes:\n%s", rawRead, raw);
                    
                    // TODO: support other responses
                    const char* resp = "HTTP/1.1 403 Forbidden\r\nServer: http_server/0.1\r\nContent-Type: text/plain\r\nContent-Length: 4\r\nConnection: close\r\n\r\n403\n";
                    send(checkedSocket, resp, strlen(resp), 0);
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
