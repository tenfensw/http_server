//
//  main.c
//  http
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "http_server.h"

/// hidden method from libhttp_server generating a good datetime str
char* hi_make_current_datetime(void);

#define SV_YES_NO(vl) (vl ? "yes" : "no")

typedef struct {
    // if true, then the web server will accept range downloads
    bool ranges;
    // if true, then the web server will be able to run CGI scripts (TBD)
    bool cgi;
    // if true, then the web server will display directory listing
    bool dirL;
    
    // IPv6 or IPv4
    bool ipv6;
    
    // IP info
    char* address;
    http_port_t port;
    
    // root static directory
    char* root;
    // the HTTP server itself
    http_server_ref server;
} sv_options;

char* sv_getwd() {
    char* result = calloc(MAXPATHLEN, sizeof(char));
    getwd(result);
    
    return result;
}

void sv_show_help() {
    fprintf(stderr, "Usage: http [-6] [-lIPADDR] [-pPORT] [-N] [-C] [-rROOT]\n");
    fprintf(stderr, "       [-D] [-help]\n");
}

sv_options sv_make_options(const size_t argc, const char** argv) {
    sv_options opts = { true, false, true, false, strdup(HTTP_ADDRESS_PUBLIC), 5454,
        sv_getwd(), NULL };
    
    for (size_t index = 1; index < argc; index++) {
        const char* param = argv[index];
        
        if (strlen(param) < 2 && param[0] != '-') {
            fprintf(stderr, "warning! invalid param - \"%s\" - must start with a dash\n",
                    param);
            continue;
        }
        
        char opt = param[1];
        param += 2;
        
        switch (opt) {
            case '6': {
                opts.ipv6 = true;
                
                if (strcmp(opts.address, HTTP_ADDRESS_PUBLIC) == 0) {
                    // adapt defaults
                    free(opts.address);
                    opts.address = strdup(HTTP_ADDRESS_PUBLIC_IPV6);
                }
                
                break;
            }
            case 'l': {
                free(opts.address);
                
                // listen on the specified IP
                opts.address = strdup(param);
                break;
            }
            case 'p': {
                // port
                opts.port = (http_port_t)atoi(param);
                break;
            }
            case 'N': {
                // ranges disabled
                opts.ranges = false;
                break;
            }
            case 'C': {
                // CGI enabled
                opts.cgi = true;
                break;
            }
            case 'r': {
                // root
                opts.root = strdup(param);
                break;
            }
            case 'D': {
                // no directory listing
                opts.dirL = false;
                break;
            }
            case 'h':
            case 'H':
            case '?': {
                // show help and exit
                sv_show_help();
                exit(1);
            }
            default: {
                fprintf(stderr, "warning! unknown option - '-%c'\n", opt);
                break;
            }
        }
    }
    
    return opts;
}

http_headers_ref sv_http_callback(const http_headers_ref request,
                                  void* additionalData) {
    char* coolDateTime = hi_make_current_datetime();
    
    // read out options as we'll need them
    sv_options* optsPtr = (sv_options*)additionalData;
    printf(" %s %s - %s %s \n", coolDateTime, http_headers_get_client_info(request),
                                http_headers_get_request_type(request),
                                http_headers_get_request_url(request));
    
    free(coolDateTime);
    
    return NULL;
}

int main(const int argc, const char** argv) {
    sv_options opts = sv_make_options((size_t)argc, argv);
    
    // init web server
    if (opts.ipv6)
        opts.server = http_server_init_ipv6(opts.address, opts.port);
    else
        opts.server = http_server_init_ipv4(opts.address, opts.port);
    
    http_server_set_callback(opts.server, sv_http_callback, &opts);
    
    // show success message
    printf("=============================================\n");
    printf(" Listening on %s:%u... \n", opts.address, opts.port);
    printf(" Press Ctrl+C to stop \n\n");
    printf(" Directory listings: %s \n", SV_YES_NO(opts.dirL));
    printf(" CGI scripts: %s \n", SV_YES_NO(opts.cgi));
    printf(" Enhanced downloads: %s \n\n", SV_YES_NO(opts.ranges));
    printf(" Served directory: %s \n", opts.root);
    printf("=============================================\n");
    
    http_server_listen(opts.server);
    return 0;
}
