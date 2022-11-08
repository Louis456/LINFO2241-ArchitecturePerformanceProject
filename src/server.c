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
#include <pthread.h>

#include "../headers/packet_implem.h"
#include "../headers/utils.h"
#include "../headers/threads.h"

const bool showDebug = false;
const opti_choice opti = BOTH_OPTI;

uint32_t file_size = 0;
uint32_t **files;

int main(int argc, char **argv) {

    struct timeval launch_time;
    if (showDebug) get_current_clock(&launch_time);
    
    char *listen_port = NULL;
    char *error = NULL;
    uint16_t nb_threads = 0;
    int opt;

    while ((opt = getopt(argc, argv, "j:s:p:")) != -1) {
        switch (opt) {
        case 'j': nb_threads = (uint16_t) strtol(optarg, &error, 10); break;
        case 's': file_size = (uint32_t) strtol(optarg, &error, 10); break;
        case 'p': listen_port = optarg; break;
        default:          
            fprintf(stdout, "Usage:\n\t%s [-j nb_thread] [-s size] [-p port]\n", argv[0]);
            exit(EXIT_FAILURE);
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

    files = malloc(sizeof(void*) * 1000); 
    if (files == NULL) fprintf(stderr, "Error malloc: files\n");
    for (uint32_t i = 0 ; i < 1000; i++) {
        files[i] = malloc(file_size*file_size*sizeof(uint32_t));
        if (files[i] == NULL) fprintf(stderr, "Error malloc: files[i]\n");
    }

    for (uint32_t i = 0; i < file_size * file_size; i++)
        files[0][i] = i;
    
    uint32_t *encrypted_file = malloc(sizeof(uint32_t) * file_size * file_size);
    if (encrypted_file == NULL) fprintf(stderr, "Error malloc: encrypted_file\n");
    
    struct sockaddr_storage their_addr; 
    socklen_t addr_size;
    int optval = 1;
    int max_connection_in_queue = 8192;

    //char s[INET6_ADDRSTRLEN];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(atoi(listen_port));


    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        fprintf(stderr, "Error while setting socket option\n errno: %d\n", errno);
        return 1;
    }
    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
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
    int client_fd;

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
                    addr_size = sizeof(their_addr);
                    client_fd = accept(sockfd, (struct sockaddr *) &servaddr, (socklen_t *) &addr_size);
                    if (client_fd == -1) fprintf(stderr, "accept failed\n errno: %d\n", errno);
                    if (showDebug) printf("Connection accepted\n");

                    pkt_request_t* pkt_request = pkt_request_new();
                    if (pkt_request == NULL) fprintf(stderr, "Error while making a new request packet\n");
                    if (recv_request_packet(pkt_request, client_fd, file_size) != PKT_OK) {
                        printf("not pkt_ok\n");
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
                        uint32_t *key = (uint32_t *) pkt_request->key;
                        uint32_t *file = files[pkt_request_get_findex(pkt_request)];
                        
                        encrypt_file(encrypted_file, file, file_size, key, pkt_request->key_size, opti);
                        uint8_t code = 0;
                        send(client_fd, &code, 1,0);
                        unsigned sz = htonl(file_size*file_size * sizeof(uint32_t));
                        send(client_fd, &sz, 4,0);
                        send(client_fd, encrypted_file, file_size*file_size * sizeof(uint32_t),0);
                    
                        pkt_request_del(pkt_request);

                        close(client_fd);                                                  
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
    for (uint32_t i = 0; i < 1000; i++) {
        free(files[i]);
    }
    free(files);
    free(encrypted_file);
    return 1;
}