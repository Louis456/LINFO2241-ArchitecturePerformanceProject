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



int main(int argc, char **argv) {
    int opt;

    uint16_t listen_port;
    uint16_t file_size;
    uint16_t nb_threads;

    // ./server -j 4 -s 1024 -p 2241
    while ((opt = getopt(argc, argv, "j:s:p:h")) != -1) {
        switch (opt) {
        case 'j': // #Threads
            nb_threads = optarg;
            break;
        case 's': // file size to be squared
            file_size = optarg;
            break;
        case 'p': // listen port
            listen_port = optarg;
        case 'h': // help
            return print_usage(argv[0]);
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind != argc) { //TODO: check and modify if needed
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    int status;
    uint16_t timeout = 4000; // timeout in ms
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *serverinfo;  // will point to the results
    int sockfd, new_fd;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET6;     //IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if ((status = getaddrinfo(NULL, listen_port, &hints, &serverinfo)) != 0) {
        ERROR("getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    sockfd = socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol);
    if (sockfd == -1) ERROR("Error while creating the socket\n errno: %d", errno);

    int binderror = bind(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen);
    if (binderror == -1) ERROR("Error while binding the socket\n errno: %d", errno);
 
    int listenerror = listen(sockfd, 5);// # connections allowed on the incoming queue.
    if (listenerror == -1) ERROR("listen failed\n errno: %d", errno);
    else printf("Server listening\n");

    // accept incoming connection
    addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

    // Let's chat with the client



    close(sockfd);

    

}