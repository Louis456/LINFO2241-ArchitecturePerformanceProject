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

#include "../headers/utils.h"
#include "../headers/threads.h"

uint32_t file_size = 0;
uint32_t file_byte_size = 0;
float **files;

int main(int argc, char **argv) {
    
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

    file_byte_size = file_size * file_size * sizeof(float);
    files = malloc(sizeof(void*) * 1000); 
    if (files == NULL) fprintf(stderr, "Error malloc: files\n");
    
    for (uint32_t i = 0 ; i < 1000; i+=4) {
        files[i] = aligned_alloc(file_size,file_byte_size);
        if (files[i] == NULL) fprintf(stderr, "Error malloc: files[i]\n");
        files[i+1] = aligned_alloc(file_size,file_byte_size);
        if (files[i+1] == NULL) fprintf(stderr, "Error malloc: files[i]\n");
        files[i+2] = aligned_alloc(file_size,file_byte_size);
        if (files[i+2] == NULL) fprintf(stderr, "Error malloc: files[i]\n");
        files[i+3] = aligned_alloc(file_size,file_byte_size);
        if (files[i+3] == NULL) fprintf(stderr, "Error malloc: files[i]\n");
    }
    

    for (uint32_t i = 0; i < file_size * file_size; i+=4) {
        files[0][i] = i;
        files[0][i+1] = i+1;
        files[0][i+2] = i+2;
        files[0][i+3] = i+3;
    }
    
    
    float *encrypted_file = aligned_alloc(file_size,file_byte_size);
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

    struct pollfd fds[max_connection_in_queue];
    fds[0].fd = sockfd; fds[0].events = POLLIN;
    int pollin_happened;
    int client_fd;
    int n_events;

    // request variables
    uint32_t key_size = 0;
    uint32_t old_key_size = 0;
    uint32_t findex = 0;
    float *key = NULL;
    int numbytes;
    uint32_t key_payload_length = 0;
    uint32_t recv_done;

    // response variables
    uint8_t code = 0;
    uint32_t sz = htonl(file_byte_size);

    while(true) {
        n_events = poll(fds, 1, -1);
        if (n_events < 0) fprintf(stderr, "Error while using poll(), errno: %d", errno);
        else if (n_events > 0){
            do {
                pollin_happened = fds[0].revents & POLLIN;
                if (pollin_happened) { //new connection on server socket
                    addr_size = sizeof(their_addr);

                    /* Accepting connection */
                    client_fd = accept(sockfd, (struct sockaddr *) &servaddr, (socklen_t *) &addr_size);
                    if (client_fd == -1) fprintf(stderr, "accept failed\n errno: %d\n", errno);

                    /* Receiving request Headers */
                    if ((numbytes = recv(client_fd, &findex, 4, 0)) == -1) 
                        fprintf(stderr, "Error while receiving header from client\n errno: %d\n", errno);
                    if ((numbytes = recv(client_fd, &key_size, 4, 0)) == -1)
                        fprintf(stderr, "Error while receiving header from client\n errno: %d\n", errno); 
                    findex = ntohl(findex); key_size = ntohl(key_size);

                    /* Receiving request Key */
                    if (old_key_size != key_size){ // malloc only once for keys of same sizes
                        if (file_size % key_size != 0) fprintf(stderr, "Invalid key format\n");
                        key_payload_length = key_size*key_size*sizeof(float);
                        if (key != NULL) free(key);
                        key = malloc(key_payload_length);
                    }
                    recv_done = 0;
                    while (recv_done < key_payload_length) {
                        if ((numbytes = recv(client_fd, key, key_payload_length - recv_done, 0)) == -1)
                            fprintf(stderr, "Error while receiving payload from client\n errno: %d\n", errno);
                        recv_done += numbytes;
                    }

                    /* Sending response */
                    encrypt_file(encrypted_file, files[findex], file_size, key, key_size);
                    if (send(client_fd, &code, 1, MSG_NOSIGNAL) == -1) fprintf(stderr, "send failed, response error_code\n errno: %d\n", errno);
                    if (send(client_fd, &sz, 4, MSG_NOSIGNAL) == -1) fprintf(stderr, "send failed, response size\n errno: %d\n", errno);
                    if (send(client_fd, encrypted_file, file_byte_size, MSG_NOSIGNAL) == -1) fprintf(stderr, "send failed, response encrypted_file\n errno: %d\n", errno);
                    close(client_fd);
                }
                n_events = poll(fds, 1, 0);
            } while (n_events > 0); // accept while there is new connections on listen queue
        }
    }
    
    close(sockfd);
    for (uint32_t i = 0; i < 1000; i++) {
        free(files[i]);
    }
    free(files);
    free(encrypted_file);
    if (key != NULL) free(key);
    return 1;
}