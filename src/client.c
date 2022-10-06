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

#define MAXDATASIZE 100

int main(int argc, char **argv) {
    int opt;

    char *server_ip_port = NULL;
    uint16_t server_port;
    char *server_port_str = NULL;
    char *server_ip = NULL;
    char *error = NULL;
    uint32_t key_size;
    uint32_t mean_rate_request;
    uint32_t duration; //in seconds
    
    fprintf(stdout, "before options\n");

    while ((opt = getopt(argc, argv, "k:r:t:h")) != -1) {
        switch (opt) {
        case 'k':
            key_size = (uint32_t) strtol(optarg,&error,10);
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
    server_port = (uint16_t) strtol(token, &error,10);    
    if (*error != '\0') {
        fprintf(stderr, "Receiver port parameter is not a number\n");
        return 1;
    }

    int sockfd;
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

    sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    if (sockfd == -1) fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);
    
    if (connect(sockfd, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
        close(sockfd);
        fprintf(stderr, "Error while connecting to server\n errno: %d\n", errno);
    }

    int numbytes;
    char buf[MAXDATASIZE];

    fprintf(stdout, "before receiving\n");

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        fprintf(stderr, "Error while receiving from server\n errno: %d\n", errno);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);

    close(sockfd);

}

// ./client -k 128 -r 1000 -t 10 127.0.0.1:2241