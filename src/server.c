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

const bool showDebug = false;

int print_usage(char *prog_name) {
    fprintf(stdout, "Usage:\n\t%s [-j nb_thread] [-s size] [-p port]\n", prog_name);
    return EXIT_FAILURE;
}

bool isNotPowerOfTwo(uint32_t file_size) {
    return (file_size <= 0) || ((file_size & (file_size - 1)) != 0);
}

void generateFiles(char ****files_ptr, uint32_t file_size) {
    srandom(42);
    if (showDebug) printf("Start generating 1000 files\n");
    *files_ptr = malloc(sizeof(char**)*1000);
    char ***files = *files_ptr;
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
    if (showDebug) printf("Files generated.\n");
}

int getAvailableThreadId(thread_status_code *thread_status, uint16_t nb_threads){
    for (int j = 0; j < nb_threads; j++) {
        if (thread_status[j] == STOPPED) return j;
    }
    return -1;
}

void createThread(
    thread_status_code *thread_status, 
    request_queue_t *queue, 
    uint16_t thread_id, 
    uint32_t file_size, 
    char ***files, 
    pthread_t *threads, 
    bool *thread_activated
) {
    thread_status[thread_id] = RUNNING;
    node_t *node = pop(queue);
    server_thread_args *args = (server_thread_args *) malloc(sizeof(server_thread_args));
    if (args == NULL) fprintf(stderr, "Error malloc server thread args\n");
    args->id = thread_id;
    args->fd = node->fd;
    args->fsize = file_size;
    args->status = &(thread_status[thread_id]);
    args->files = files;
    pthread_create(&threads[thread_id], NULL, &start_server_thread, (void*) args);
    thread_activated[thread_id] = true;
    free(node);
}


int main(int argc, char **argv) {

    struct timeval launch_time;
    if (showDebug) get_current_clock(&launch_time);

    int opt;
    char *listen_port = NULL;
    uint32_t file_size = 0;
    uint16_t nb_threads = 0;
    char *error = NULL;

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

    if (isNotPowerOfTwo(file_size)) {
        fprintf(stderr, "File size must be a power of 2\n");
        return 1;
    }

    char ***files = NULL;
    generateFiles(&files, file_size);

    int status;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *serverinfo; 
    int sockfd;
    int optval = 1;
    int max_connection_in_queue = 1000;

    char s[INET6_ADDRSTRLEN];

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

    // Init thread pool and its status
    pthread_t threads[nb_threads];
    bool thread_activated[nb_threads];
    thread_status_code thread_status[nb_threads]; 
    for (int j = 0; j < nb_threads; j++) thread_activated[j] = false;
    for (int j = 0; j < nb_threads; j++) thread_status[j] = STOPPED;

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
                    if (showDebug) printf("Connection accepted\n");

                    inet_ntop(their_addr.ss_family, 
                        &(((struct sockaddr_in *)&their_addr))->sin_addr,
                        s, sizeof s);

                    if (new_fd != -1) {
                        push(&queue, new_fd);
                        if (showDebug) printf("Packet added, queue size: %d\n", queue.size);
                    }

                    // Check if there is an available thread
                    int thread_id = getAvailableThreadId(thread_status, nb_threads);
                    if (thread_id != -1 && !isEmpty(&queue)) {
                        createThread(thread_status, &queue, thread_id, file_size, files, threads, thread_activated);
                    }
                }
                n_events = poll(fds, 1, 0);
            } while (n_events > 0); // accept while there is new connections on listen queue
            
        } else {
             // Check if there is an available thread
            int thread_id = getAvailableThreadId(thread_status, nb_threads);
            if (thread_id != -1 && !isEmpty(&queue)) {
                createThread(thread_status, &queue, thread_id, file_size, files, threads, thread_activated);
            }
        }
        // get time diff from last request received and now
        get_current_clock(&now);
        timersub(&now, &start_time, &diff_time);
    }
    
    // Close and free the server
    for (uint16_t j = 0; j < nb_threads; j++){
        if (thread_activated[j]) pthread_join(threads[j], NULL);
    }
    close(sockfd);
    freeaddrinfo(serverinfo);
    for (int i = 0; i < 1000; i++) {
        for (uint32_t j = 0; j < file_size; j++) {
            free(files[i][j]);
        }
        free(files[i]);
    }
    free(files);
    return 1;
}