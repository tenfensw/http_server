//
//  main.c
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include "http_server.h"

int main() {
    http_server_ref server = http_server_init_ipv4(HTTP_ADDRESS_PUBLIC, 5454);
    http_server_listen(server);
    http_server_release(server);
    return 0;
}
