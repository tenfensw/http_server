//
//  main.c
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include "http_server.h"

http_headers_ref main_callback(const http_headers_ref request,
                               void* nullPlaceholder) {
    // not using it here yet
    (void)(nullPlaceholder);
    
    if (strcmp(http_headers_get_request_type(request), "GET") != 0)
        return http_headers_init_with_response(400, "text/plain", strdup("bad"), 2, free);
    
    return http_headers_init_with_response(200, "text/html", strdup("<title>hi</title>"), 17, free);
}

int main() {
    http_server_ref server = http_server_init_ipv6(HTTP_ADDRESS_PUBLIC_IPV6, 5454);
    http_server_set_callback(server, main_callback, NULL);
    http_server_listen(server);
    http_server_release(server);
    return 0;
}
