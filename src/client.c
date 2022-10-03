#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <limits.h>

int main(int argc, char **argv) {
    int opt;


    char *filename = NULL;
    char *stats_filename = NULL;
    char *server_ip_port = NULL;
    char *receiver_port_err;
    uint16_t receiver_port;
    uint32_t key_size;
    uint32_t mean_rate_request;
    uint32_t duration; //in seconds

    while ((opt = getopt(argc, argv, "k:r:th")) != -1) {
        switch (opt) {
        case 'k':
            key_size = optarg;
            break;
        case 'r':
            mean_rate_request = optarg;
            break;
        case 't':
            duration = optarg;
            duration *= 1000;
            break;
        case 'h': // help
            return print_usage(argv[0]);
        default:
            return print_usage(argv[0]);
        }
    }

    server_ip_port = argv[optind];
    server_port = (uint16_t) strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

}



// ./client -k 128 -r 1000 -t 10 127.0.0.1:2241