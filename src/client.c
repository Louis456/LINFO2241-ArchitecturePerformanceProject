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
#include <sys/poll.h>
#include <arpa/inet.h>
#include <limits.h>
#include <pthread.h>

#include "../headers/packet_implem.h"
#include "../headers/utils.h"

int main(int argc, char **argv) {
    int opt;
    srandom(time(NULL)); // initialse random generator

    char *server_ip_port = NULL;
    //uint16_t server_port;
    char *server_port_str = NULL;
    char *server_ip = NULL;
    char *error = NULL;
    uint32_t mean_rate_request = 0;
    uint32_t duration = 0; //in ms
    uint32_t key_size = 0;
    uint64_t key_payload_length = 0; // key_size squared
    
    fprintf(stdout, "before options\n");

    while ((opt = getopt(argc, argv, "k:r:t:h")) != -1) {
        switch (opt) {
        case 'k':
            key_size = (uint32_t) strtol(optarg,&error,10);
            key_payload_length = key_size * key_size;
            if (*error != '\0') {
                fprintf(stderr, "key size is not a number\n");
                return 1;
            }
            break;
        case 'r':
            mean_rate_request = (uint32_t) strtol(optarg,&error,10);
            if (*error != '\0') {
                fprintf(stderr, "mean rate of request/s is not a number\n");
                return 1;
            }
            break;
        case 't':
            duration = (uint32_t) strtol(optarg,&error,10);
            if (*error != '\0') {
                fprintf(stderr, "duration is not a number\n");
                return 1;
            }
            duration *= 1000;
            break;
        case 'h': // help
            return 1;
        default:
            return 1;
        }
    }

    fprintf(stdout, "after options\n");

    const char * separator = ":";
    server_ip_port = argv[optind];
    char * token = strtok(server_ip_port,separator);
    server_ip = token;
    token = strtok(NULL, separator);
    server_port_str = token;
    //server_port = (uint16_t) strtol(token, &error,10);    
    if (*error != '\0') {
        fprintf(stderr, "Receiver port parameter is not a number\n");
        return 1;
    }

    int status;   
    struct addrinfo hints;
    struct addrinfo *serverinfo;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;     //IPv6
    hints.ai_socktype = SOCK_STREAM; // UDP Datagram sockets
    hints.ai_protocol = 0;

    // By using the AI_PASSIVE flag, I’m telling the program to bind to the IP of the host it’s running on.
    if ((status = getaddrinfo(server_ip, server_port_str, &hints, &serverinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    }

    struct timeval start_time;
    struct timeval now;
    struct timeval diff_time;
    get_current_clock(&start_time);
    get_current_clock(&now);
    timersub(&now, &start_time, &diff_time);

    client_thread_args *args = (client_thread_args *) malloc(sizeof(client_thread_args));
    args->serverinfo = serverinfo;
    args->key_size = key_size;
    args->key_payload_length = key_payload_length;
    while (get_ms(&diff_time) < duration) {
        // Start a client thread
        pthread_t thread;
        int pthread_err = pthread_create(&thread, NULL, &start_client, (void*) args);
        if (pthread_err != 0) fprintf(stderr, "Error while creating a thread\n");

        // Sleep following a normal distribution with Box-Muller algorithm
        uint32_t mu = (1/mean_rate_request)*1000000;
        uint32_t sigma = 0.1*mu;
        uint32_t time_to_sleep = get_gaussian_number(mu, sigma);
        int errsleep = usleep(time_to_sleep); // time in microseconds
        if (errsleep == -1) fprintf(stderr, "Error while nanosleeping\n errno: %d\n", errno);
        
        // Just before looping again, check current time and get diff from start
        get_current_clock(&now);
        timersub(&now, &start_time, &diff_time);
    }
    
}