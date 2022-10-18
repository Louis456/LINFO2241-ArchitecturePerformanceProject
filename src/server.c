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

int print_usage(char *prog_name) {
    fprintf(stdout, "Usage:\n\t%s [-j nb_thread] [-s size] [-p port]\n", prog_name);
    return EXIT_FAILURE;
}

int main(int argc, char **argv) {
    int opt;
    char *listen_port;
    uint32_t file_size;
    uint16_t nb_threads;
    char *error = NULL;
    printf("before options\n");
    // ./server -j 4 -s 1024 -p 2241
    while ((opt = getopt(argc, argv, "j:s:p:h")) != -1) {
        switch (opt) {
        case 'j': // #Threads
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

    printf("after options\n");

    if (optind != argc) { //TODO: check and modify if needed
        fprintf(stderr, "Unexpected number of positional arguments\n");
        return 1;
    }

    if ((file_size <= 0) || ((file_size & (file_size - 1)) != 0)) { // Check file_size is a power of 2
        fprintf(stderr, "File size must be a power of 2\n");
        return 1;
    }

    srandom(42); // initialse random generator

    printf("Start generating 1000 files\n");
    char ***files = malloc(sizeof(char**)*1000);
    if (files == NULL) fprintf(stderr, "Error malloc file\n");
    for (int i = 0; i < 1000; i++) {
        files[i] = malloc(sizeof(char*)*file_size);
        if (files[i] == NULL) fprintf(stderr, "Error malloc file\n");
        for (uint32_t j = 0; j < file_size; j++) {
            files[i][j] = malloc(sizeof(char)*file_size);
            if (files[i][j] == NULL) fprintf(stderr, "Error malloc file\n");
            for (uint32_t k = 0; k < file_size; k++) {
                char r = 1 + (random() % 255);
                files[i][j][k] = r;
            }
        }
    }
    printf("Files generated.\n");


    int status;
    //uint16_t timeout = 4000; // timeout in ms
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *serverinfo;  // will point to the results
    int sockfd;
    int optval = 1;
    int sockopt;

    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;     //IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if ((status = getaddrinfo(NULL, listen_port, &hints, &serverinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    sockfd = socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol);
    if (sockfd == -1) fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);

    sockopt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (sockopt == -1) fprintf(stderr, "Error while setting socket option\n errno: %d\n", errno);
    sockopt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (sockopt == -1) fprintf(stderr, "Error while setting socket option\n errno: %d\n", errno);

    int binderror = bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen);
    if (binderror == -1) fprintf(stderr, "Error while binding the socket\n errno: %d\n", errno);
 
    int listenerror = listen(sockfd, 1000);// # connections allowed on the incoming queue.
    if (listenerror == -1) fprintf(stderr, "listen failed\n errno: %d\n", errno);
    else printf("Server listening\n");

    printf("EVENT server_listening\n");


    // Init thread pool and its status
    pthread_t threads[nb_threads];
    bool thread_activated[nb_threads];
    for (int j = 0; j < nb_threads; j++) {
        thread_activated[j] = false;
    }
    thread_status_code thread_status[nb_threads]; 
    for (int j = 0; j < nb_threads; j++) {
        thread_status[j] = STOPPED;
    }

    struct timeval start_time;
    struct timeval now;
    struct timeval diff_time;
    get_current_clock(&start_time);
    get_current_clock(&now);
    timersub(&now, &start_time, &diff_time);

    // Create a queue
    request_queue_t queue;
    struct pollfd fds[1000];
    int pollin_happened;
    fds[0].fd = sockfd; fds[0].events = POLLIN;
    int new_fd;

    while(!(get_ms(&diff_time) > 7000 && isEmpty(&queue))) {  // main accept() loop
        int n_events = poll(fds, 1, 0);
        if (n_events < 0) fprintf(stderr,"Error while using poll(), errno: %d", errno);
        else if (n_events > 0) {         
            do {
                pollin_happened = fds[0].revents & POLLIN;
                if (pollin_happened) { //new connection on server socket

                    get_current_clock(&start_time); // reset timer upon new request
                    addr_size = sizeof their_addr;

                    new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size);
                    if (new_fd == -1) fprintf(stderr, "accept failed\n errno: %d\n", errno);
                    printf("Connection accepted\n");

                    inet_ntop(their_addr.ss_family, 
                        &(((struct sockaddr_in *)&their_addr))->sin_addr,
                        s, sizeof s);
                    //printf("got connection from %s\n", s);
                    //printf("packet file index : %u\n",pkt_request_get_findex(pkt_request));
                    //printf("packet key size : %u\n",pkt_request_get_ksize(pkt_request));
                    //printf("packet key : %s\n",pkt_request_get_key(pkt_request));
                    if (new_fd != -1) push(&queue, new_fd);
                    printf("Packet added, queue size: %d\n", queue.size);

                    // Check if there is an available thread
                    for (int j = 0; j < nb_threads; j++) {
                        if (thread_status[j] == STOPPED && !isEmpty(&queue)) {
                            thread_status[j] = RUNNING;
                            node_t *node = pop(&queue);
                            server_thread_args *args = (server_thread_args *) malloc(sizeof(server_thread_args));
                            if (args == NULL) fprintf(stderr, "Error malloc server thread args\n");
                            args->id = j;
                            args->fd = node->fd;
                            args->fsize = file_size;
                            args->status = &(thread_status[j]);
                            args->files = files;
                            pthread_create(&threads[j], NULL, &start_server_thread, (void*) args);
                            thread_activated[j] = true;
                            //printf("Thread %d started\n", j);
                            free(node);
                        } 
                    }
                }
                n_events = poll(fds, 1, 0);
                

            } while (n_events > 0); // accept while there is new connections on listen queue
            
        } else {
            for (int j = 0; j < nb_threads; j++) {
            if (thread_status[j] == STOPPED && !isEmpty(&queue)) {
                thread_status[j] = RUNNING;
                node_t *node = pop(&queue);
                server_thread_args *args = (server_thread_args *) malloc(sizeof(server_thread_args));
                if (args == NULL) fprintf(stderr, "Error malloc server thread args\n");
                args->id = j;
                args->fd = node->fd;
                args->fsize = file_size;
                args->status = &(thread_status[j]);
                args->files = files;
                pthread_create(&threads[j], NULL, &start_server_thread, (void*) args);
                thread_activated[j] = true;
                //printf("Thread %d started\n", j);
                free(node);
            } 
        }
        }
        
        // get time diff from last request received and now
        get_current_clock(&now);
        timersub(&now, &start_time, &diff_time);
    }
    
    for (uint16_t j = 0; j < nb_threads; j++){
        if (thread_activated[j]) pthread_join(threads[j], NULL);
    }

    close(sockfd);
    // Free files
    for (int i = 0; i < 1000; i++) {
        for (uint32_t j = 0; j < file_size; j++) {
            free(files[i][j]);
        }
        free(files[i]);
    }
    free(files);

    return 1;
}