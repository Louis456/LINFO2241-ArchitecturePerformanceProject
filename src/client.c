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

#include "packet_implem.h"

void create_pkt_request(pkt_request_t* pkt, uint32_t findex, uint32_t ksize, char *key)
{
    pkt_status_code status_code = pkt_request_set_findex(pkt, findex);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file index in request packet");

    status_code = pkt_request_set_ksize(pkt, ksize);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the key size in request packet");

    uint64_t key_length = ksize * ksize;
    status_code = pkt_request_set_key(pkt, key, key_length);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the key in request packet");
}

void get_current_clock(struct timeval *timestamp) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		//ERROR("Cannot get internal clock");
	}
    timestamp->tv_sec = ts.tv_sec;
	timestamp->tv_usec = ts.tv_nsec/1000;
}

uint64_t get_ms(struct timeval *timestamp) {
    return timestamp->tv_sec * 1000 + timestamp->tv_usec / 1000;
}

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
    char buf[MAX_RESPONSE_LENGTH];

    fprintf(stdout, "before receiving\n");

    if ((numbytes = recv(sockfd, buf, MAX_RESPONSE_LENGTH-1, 0)) == -1) {
        fprintf(stderr, "Error while receiving from server\n errno: %d\n", errno);
    }

    pkt_response_t* pkt = pkt_response_new();
    pkt_response_decode(buf,pkt);
    

    printf("error code: '%hhu'\n",pkt_response_get_errcode(pkt));
    printf("file size: '%u'\n",pkt_response_get_fsize(pkt));
    printf("error code: '%s'\n",pkt_response_get_file(pkt));

    close(sockfd);

}

// ./client -k 128 -r 1000 -t 10 127.0.0.1:2241