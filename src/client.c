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
#include <math.h>
#include <pthread.h>

#include "../headers/utils.h"
#include "../headers/threads.h"

int print_usage(char *prog_name) {
    fprintf(stdout, "Usage:\n\t%s [-k key_size] [-r request_rate] [-t duration] serverip:port\n", prog_name);
    return EXIT_FAILURE;
}

const bool showDebug = false;

int main(int argc, char **argv) {

    printf("unroll 1");

    srandom(time(NULL));

    char *server_ip_port = NULL;
    char *server_port_str = NULL;
    char *error = NULL;
    uint32_t mean_rate_request = 0;
    uint32_t duration = 0; //in ms
    uint32_t key_size = 0;
    uint64_t key_payload_length = 0; // key_size squared
    int opt;

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
            return print_usage(argv[0]);
        default:
            return print_usage(argv[0]);
        }
    }

    const char * separator = ":";
    server_ip_port = argv[optind];
    char * token = strtok(server_ip_port,separator);
    token = strtok(NULL, separator);
    server_port_str = token;
    if (*error != '\0') {
        fprintf(stderr, "Receiver port parameter is not a number\n");
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    // Filling server information
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(atoi(server_port_str)); 
    servaddr.sin_addr.s_addr = INADDR_ANY;
    

    struct timeval start_time;
    struct timeval now;
    struct timeval diff_time;
    get_current_clock(&start_time);
    get_current_clock(&now);
    timersub(&now, &start_time, &diff_time);

    uint32_t nb_available_threads = 1000000;
    uint32_t thread_id = 0;
    pthread_t *threads = malloc(sizeof(pthread_t) * nb_available_threads); //takes 4mo of RAM
    if (threads == NULL) fprintf(stderr, "Error malloc threads\n");

    while (get_ms(&diff_time) < duration) {
        // Start a client thread
        client_thread_args *args = (client_thread_args *) malloc(sizeof(client_thread_args));
        if (args == NULL) fprintf(stderr, "Error malloc: client_thread_args\n");
        args->servaddr = &servaddr;
        args->key_size = key_size;
        args->key_payload_length = key_payload_length;

        int pthread_err = pthread_create(&(threads[thread_id]), NULL, &start_client_thread, (void*) args);
        if (pthread_err != 0) fprintf(stderr, "Error while creating a thread\n");
        thread_id++;
        
        double time_to_sleep = 1000000.0 / ((double) mean_rate_request);
        int errsleep = usleep(time_to_sleep); // time in microseconds
        if (errsleep == -1) fprintf(stderr, "Error while usleeping\n errno: %d\n", errno);
        
        // Just before looping again, check current time and get diff from start
        get_current_clock(&now);
        timersub(&now, &start_time, &diff_time);
    }

    if (showDebug) printf("waiting for thread to join\n");

    for (uint32_t i = 0; i < thread_id; i++){
        pthread_join(threads[i], NULL);
    }

    // Free
    free(threads);
}