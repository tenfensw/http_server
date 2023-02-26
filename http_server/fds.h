//
//  fds.h
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#pragma once

#include "wrappers.h"

//
// compared to other private headers, http_fd_set_ref's implementation is entirely
// hidden in fds.c, while it's protected methods are declared here
//
// this is because http_fd_set_ref is not supposed to be used publicly at all
//

http_fd_set_ref http_fd_set_init(const http_size_t maxClients);

/// set main server socket serving the clients
void http_fd_set_set_main_socket(http_fd_set_ref set, int sk);
/// adds the specified socket to the fd_set and the array
bool http_fd_set_add(http_fd_set_ref set, int sk, bool doFdSet);
/// sync client connections with the fd_set
bool http_fd_set_sync_descriptors(http_fd_set_ref set);

bool http_fd_set_select(http_fd_set_ref set);
bool http_fd_set_is_set(http_fd_set_ref set, int sk);

int http_fd_set_get_socket(http_fd_set_ref set, const http_size_t index);
bool http_fd_set_nullify_socket(http_fd_set_ref set, const http_size_t index);

void http_fd_set_release(http_fd_set_ref set);
