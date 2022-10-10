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
    uint16_t file_size;
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
            file_size = (uint16_t) strtol(optarg, &error, 10);
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

    srandom(42); // initialse random generator

    printf("Start generating 1000 files\n");
    uint8_t ***files = malloc(sizeof(uint8_t**)*1000);
    for (int i = 0; i < 1000; i++) {
        files[i] = malloc(sizeof(uint8_t*)*file_size);
        for (int j = 0; j < file_size; j++) {
            files[i][j] = malloc(sizeof(uint8_t)*file_size);
            for (int k = 0; k < file_size; k++) {
                uint8_t r = 1 + (random() % 255);
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
    int sockfd, new_fd;
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

    int binderror = bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen);
    if (binderror == -1) fprintf(stderr, "Error while binding the socket\n errno: %d\n", errno);
 
    int listenerror = listen(sockfd, 5);// # connections allowed on the incoming queue.
    if (listenerror == -1) fprintf(stderr, "listen failed\n errno: %d\n", errno);
    else printf("Server listening\n");

    char buf[528];

    while(1) {  // main accept() loop
        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) fprintf(stderr, "accept failed\n errno: %d\n", errno);
        printf("Connection accepted\n");

        inet_ntop(their_addr.ss_family, 
            &(((struct sockaddr_in *)&their_addr))->sin_addr,
            s, sizeof s);
        printf("got connection from %s\n", s);

        pkt_request_t* pkt_request = pkt_request_new();
        recv_request_packet(pkt_request, new_fd);

        printf("packet file index : %u\n",pkt_request_get_findex(pkt_request));
        printf("packet key size : %u\n",pkt_request_get_ksize(pkt_request));
        printf("packet key : %s\n",pkt_request_get_key(pkt_request));

        close(new_fd);

    }
        
        /*uint8_t code = 0;
        uint32_t fsize = 13; 
        char* payload = "Hello, world!";
        pkt_response_t* pkt = pkt_response_new();
        create_pkt_response(pkt, code, fsize, payload);
        pkt_response_encode(pkt, buf);
        if (send(new_fd, buf, fsize + RESPONSE_HEADER_LENGTH, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);*/


    // Free files
    for (int i = 0; i < 1000; i++) {
        free(files[i]);
        for (int j = 0; j < file_size; j++) {
            free(files[i][j]);
        }
    }
    free(files);
    
}