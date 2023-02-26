//
//  fds.c
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include "fds.h"

#ifdef __APPLE__
// to fix the warnings

int select(int, fd_set* restrict, fd_set* restrict, fd_set* restrict,
           struct timeval* restrict);
#endif

//
// private
//

struct http_fd_set_s {
    // the fd_set being wrapped itself
    fd_set managedFDs;
    
    // client connection sockets array
    int* clientFDs;
    // array size
    http_size_t clientMax;
    
    // server socket
    int mainSocket;
    
    // newest-most socket value, needed for select()
    int maxFD;
};

void http_fd_set_fill_gap_in_array(http_fd_set_ref set, int sk) {
    if (!set)
        return;
    
    for (http_size_t sz = 0; sz < set->clientMax; sz++) {
        if (set->clientFDs[sz] == 0) {
            // add it to the empty spot
            set->clientFDs[sz] = sk;
            break;
        }
    }
}

//
// public
//

http_fd_set_ref http_fd_set_init(const http_size_t maxClients) {
    if (maxClients < 1)
        return http_fd_set_init(1);
    
    http_fd_set_ref result = hizalloc_struct(http_fd_set_s);
    
    // initialize array first
    result->clientMax = maxClients;
    result->clientFDs = calloc(result->clientMax, sizeof(int));
    
    // clean the fd_set and initialize it too
    FD_ZERO(&result->managedFDs);
    
    return result;
}

void http_fd_set_set_main_socket(http_fd_set_ref set, int sk) {
    if (!set || sk < 1) {
        HI_DEBUG("invalid set pointer <%p> or socket value <%d>, refusing to set anything",
                 set, sk);
        return;
    }
    
    set->mainSocket = sk;
}

bool http_fd_set_add(http_fd_set_ref set, int sk, bool doFdSet) {
    if (!set)
        return false;
    else if (sk == 0) {
        HI_DEBUG("NULL socket is an invalid socket, cannot add it to the fd_set <%p>", set);
        return false;
    }
    
    // register it with the set if necessary
    if (doFdSet)
        FD_SET(sk, &set->managedFDs);
    
    // add it to the array too
    http_fd_set_fill_gap_in_array(set, sk);
    
    return true;
}

int http_fd_set_get_socket(http_fd_set_ref set, const http_size_t index) {
    if (!set) {
        HI_DEBUG("NULL set specified, cannot continue");
        return -1;
    } else if (index >= set->clientMax) {
        HI_DEBUG("index out of bounds, cannot continue, index = %u", index);
        return -1;
    }
    
    return set->clientFDs[index];
}

bool http_fd_set_nullify_socket(http_fd_set_ref set, const http_size_t index) {
    if (http_fd_set_get_socket(set, index) >= 0) {
        set->clientFDs[index] = 0;
        return true;
    }
    
    return false;
}

bool http_fd_set_sync_descriptors(http_fd_set_ref set) {
    if (!set)
        return false;
    
    // clean the fd_set
    FD_ZERO(&set->managedFDs);
    
    // read main socket to the fd_set
    FD_SET(set->mainSocket, &set->managedFDs);
    set->maxFD = set->mainSocket;
    
    for (http_size_t sz = 0; sz < set->clientMax; sz++) {
        int clientSocket = set->clientFDs[sz];
        
        if (clientSocket > 0) {
            FD_SET(clientSocket, &set->managedFDs);
            
            // new max socket
            if (clientSocket > set->maxFD)
                set->maxFD = clientSocket;
        }
    }
    
    return true;
}

bool http_fd_set_select(http_fd_set_ref set) {
    if (!set) {
        HI_DEBUG("NULL set provided as a parameter to select()");
        return false;
    }
    
    int result = select(set->maxFD + 1, &set->managedFDs, NULL, NULL, NULL);
    if (result < 0 && errno != EINTR) {
        HI_ERRNO_DEBUG("select failed");
        return false;
    }
    
    return true;
}

bool http_fd_set_is_set(http_fd_set_ref set, int sk) {
    if (!set)
        return false;
    
    return FD_ISSET(sk, &set->managedFDs);
}

void http_fd_set_release(http_fd_set_ref set) {
    if (!set)
        return;
    
    // first go through the list and terminate all potentially unclosed
    // connections
    for (http_size_t sz = 0; sz < set->clientMax; sz++) {
        if (set->clientFDs[sz] != 0) {
            close(set->clientFDs[sz]);
            set->clientFDs[sz] = 0;
        }
    }
    
    // destroy now clean array
    free(set->clientFDs);
    
    // zero-out all managed file descriptors
    FD_ZERO(&set->managedFDs);
    
    free(set);
}
