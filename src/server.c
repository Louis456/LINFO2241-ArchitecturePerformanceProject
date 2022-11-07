#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <time.h>

#include "../headers/packet_implem.h"
#include "../headers/utils.h"
#include "../headers/threads.h"

const bool showDebug = false;
const opti_choice opti = INLINING;

uint32_t file_size = 0;
uint32_t *files;

int print_usage(char *prog_name) {
    fprintf(stdout, "Usage:\n\t%s [-j nb_thread] [-s size] [-p port]\n", prog_name);
    return EXIT_FAILURE;
}

int main(int argc, char **argv) {

    struct timeval launch_time;
    if (showDebug) get_current_clock(&launch_time);
    
    char *listen_port = NULL;
    char *error = NULL;
    uint16_t nb_threads = 0;
    int opt;

    while ((opt = getopt(argc, argv, "j:s:p:h")) != -1) {
        switch (opt) {
        case 'j': // # Threads
            nb_threads = (uint16_t) strtol(optarg, &error, 10);
            break;
        case 's': // file size to be squared
            file_size = (uint32_t) strtol(optarg, &error, 10);
            break;
        case 'p': // listen port
            listen_port = optarg;
            break;
        case 'h':          
            return print_usage(argv[0]);
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind != argc) { 
        fprintf(stderr, "Unexpected number of positional arguments\n");
        return 1;
    }

     if (nb_threads != 1) {
        fprintf(stderr, "Unexpected number of threads\n");
        return 0;
    }

    if ((file_size <= 0) || ((file_size & (file_size - 1)) != 0)) {
        fprintf(stderr, "File size must be a power of 2\n");
        return 1;
    }

    files = (u_int32_t *) malloc(sizeof(uint32_t) * 1000 * file_size * file_size);
    if (files == NULL) fprintf(stderr, "Error malloc file\n");

    
    struct sockaddr_storage their_addr; 
    struct addrinfo hints;
    struct addrinfo *serverinfo; 
    socklen_t addr_size;
    int status;
    int sockfd;
    int optval = 1;
    int max_connection_in_queue = 8192;

    //char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if ((status = getaddrinfo(NULL, listen_port, &hints, &serverinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);
        return 1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        fprintf(stderr, "Error while setting socket option\n errno: %d\n", errno);
        return 1;
    }
    if (bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
        fprintf(stderr, "Error while binding the socket\n errno: %d\n", errno);
        return 1;
    }
    if (listen(sockfd, max_connection_in_queue) == -1) {
        fprintf(stderr, "listen failed\n errno: %d\n", errno);
        return 1;
    } else printf("server listening\n");

    if (showDebug) {
        struct timeval listen_time;
        get_current_clock(&listen_time);
        struct timeval diff_time_listen;
        timersub(&listen_time, &launch_time, &diff_time_listen);
        printf("Time to start listening in seconds: %f\n", ((double) get_ms(&diff_time_listen)) / 1000);
    }
    
    struct timeval start_time;
    struct timeval now;
    struct timeval diff_time;
    get_current_clock(&start_time);

    struct pollfd fds[max_connection_in_queue];
    fds[0].fd = sockfd; fds[0].events = POLLIN;
    int pollin_happened;
    int new_fd;

    bool first_iter = true;

    while(first_iter || get_ms(&diff_time) < 2000) {
        int n_events = poll(fds, 1, 0);
        if (n_events < 0) fprintf(stderr, "Error while using poll(), errno: %d", errno);
        else if (n_events > 0){
            first_iter = false;
            do {
                pollin_happened = fds[0].revents & POLLIN;
                if (pollin_happened) { //new connection on server socket

                    get_current_clock(&start_time); // reset timer upon new request
                    addr_size = sizeof their_addr;

                    new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size);
                    if (new_fd == -1) fprintf(stderr, "accept failed\n errno: %d\n", errno);
                    if (showDebug) printf("Connection accepted\n");


                    if (new_fd != -1) {
                        pkt_request_t* pkt_request = pkt_request_new();
                        if (pkt_request == NULL) fprintf(stderr, "Error while making a new request packet\n");
                        if (recv_request_packet(pkt_request, new_fd, file_size) != PKT_OK) {
                            // Invalid key size
                            /*
                            fprintf(stderr, "Key size must divide the file size\n");
                            pkt_response_t* pkt_response = pkt_response_new();
                            if (pkt_response== NULL) fprintf(stderr, "Error while making a new response packet in start_server_thread\n");
                            char error_message[] = "invalid key size";
                            uint8_t error_message_size = strlen(error_message)+1;
                            uint8_t error_code = 1;
                            create_pkt_response(pkt_response, error_code, error_message_size, error_message);
                            u_int8_t total_size = RESPONSE_HEADER_LENGTH + error_message_size;
                            char* buf = malloc(sizeof(char) * total_size);
                            if (buf == NULL) fprintf(stderr, "Error malloc: buf response invalid key\n");
                            if (send(arguments->fd, buf, total_size, 0) == -1) fprintf(stderr, "send failed with invalid key size\n errno: %d\n", errno);
                            free(buf);
                            pkt_response_del(pkt_response);
                            */
                        } else {
                            uint32_t* key = (uint32_t *) pkt_request->key;
                            uint32_t *file = &(files[file_size * file_size * pkt_request_get_findex(pkt_request)]); // get file at requested index
                            uint32_t* encrypted_file = malloc(sizeof(uint32_t) * file_size * file_size);
                            if (encrypted_file == NULL) fprintf(stderr, "Error malloc: encrypted_file\n");
                            encrypt_file(encrypted_file, file, file_size, key, pkt_request->key_size, opti); 
                            pkt_request_del(pkt_request);

                            // Create response packet
                            pkt_response_t* pkt_response = pkt_response_new();
                            if (pkt_response== NULL) fprintf(stderr, "Error while making a new response packet in start_server_thread\n");
                            uint8_t code = 0;
                            create_pkt_response(pkt_response, code, file_size*file_size, encrypted_file);

                            // Encode buffer and send it
                            uint32_t total_size = (file_size * file_size * sizeof(uint32_t)) + RESPONSE_HEADER_LENGTH;
                            char* buf = malloc(sizeof(char) * total_size);
                            if (buf == NULL) fprintf(stderr, "Error malloc: buf response\n");
                            pkt_response_encode(pkt_response, buf);
                            if (send(new_fd, buf, total_size, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

                            // Free and close
                            pkt_response_del(pkt_response);
                            close(new_fd);
                            free(encrypted_file);
                            free(buf);                                                            
                        }
                    }
                
                }
                n_events = poll(fds, 1, 0);
            } while (n_events > 0); // accept while there is new connections on listen queue
        }
        // get time diff from last request received and now
        get_current_clock(&now);
        timersub(&now, &start_time, &diff_time);
    }
    
    close(sockfd);
    freeaddrinfo(serverinfo);
    free(files);
    return 1;
}