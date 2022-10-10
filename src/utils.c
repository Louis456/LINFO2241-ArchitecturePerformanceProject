#include "../headers/utils.h"


void create_pkt_response(pkt_response_t* pkt, pkt_error_code code, uint32_t fsize, char* file)
{
    pkt_status_code status_code = pkt_response_set_errcode(pkt, code);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the error code in response packet");

    status_code = pkt_response_set_fsize(pkt, fsize);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file size in response packet");

    status_code = pkt_response_set_file(pkt, file, fsize);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file payload in response packet");
}

void recv_request_packet(pkt_request_t* pkt, int sockfd) {
    char buf_header[REQUEST_HEADER_LENGTH+1];
    int numbytes;
    if ((numbytes = recv(sockfd, buf_header, REQUEST_HEADER_LENGTH, 0)) == -1) {
        fprintf(stderr, "Error while receiving header from client\n errno: %d\n", errno);
    }
    uint32_t findex = ((uint32_t *) buf_header)[0];
    uint32_t ksize = ((uint32_t *) buf_header)[1];

    char buf_key[ksize*ksize];
    if ((numbytes = recv(sockfd, buf_key, ksize*ksize, 0)) == -1) {
        fprintf(stderr, "Error while receiving payload from client\n errno: %d\n", errno);
    }
    create_pkt_request(pkt, findex, ksize, buf_key);
}

void create_pkt_request(pkt_request_t* pkt, uint32_t findex, uint32_t ksize, char *key)
{
    pkt_status_code status_code = pkt_request_set_findex(pkt, findex);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file index in request packet");

    status_code = pkt_request_set_ksize(pkt, ksize);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the key size in request packet");

    uint64_t key_payload_length = ksize * ksize;
    status_code = pkt_request_set_key(pkt, key, key_payload_length);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the key in request packet");
}

void recv_response_packet(pkt_response_t* pkt, int sockfd) {
    char buf_header[RESPONSE_HEADER_LENGTH];
    int numbytes;
    if ((numbytes = recv(sockfd, buf_header, RESPONSE_HEADER_LENGTH, 0)) == -1) {
        fprintf(stderr, "Error while receiving header from server\n errno: %d\n", errno);
    }
    uint32_t errcode = ((uint8_t *) buf_header)[0];
    uint32_t fsize = ((uint32_t *) buf_header+1)[0];

    char buf_file[fsize];
    if ((numbytes = recv(sockfd, buf_file, fsize, 0)) == -1) {
        fprintf(stderr, "Error while receiving file from server\n errno: %d\n", errno);
    }
    create_pkt_response(pkt, errcode, fsize, buf_file);
}

void get_current_clock(struct timeval *timestamp) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		fprintf(stderr, "Cannot get internal clock");
	}
    timestamp->tv_sec = ts.tv_sec;
	timestamp->tv_usec = ts.tv_nsec/1000;
}

uint64_t get_ms(struct timeval *timestamp) {
    return timestamp->tv_sec * 1000 + timestamp->tv_usec / 1000;
}

void* start_client(void* args) {
    client_thread_args *arguments = (client_thread_args *) args;
    int sockfd = socket(arguments->serverinfo->ai_family, arguments->serverinfo->ai_socktype, arguments->serverinfo->ai_protocol);
    if (sockfd == -1) fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);
    
    if (connect(sockfd, arguments->serverinfo->ai_addr, arguments->serverinfo->ai_addrlen) == -1) {
        close(sockfd);
        fprintf(stderr, "Error while connecting to server\n errno: %d\n", errno);
    }
    
    // Generate random key and file index
    printf("Start generating a random key\n");
    uint8_t *key = malloc(sizeof(uint8_t)*arguments->key_payload_length);       
    for (uint64_t i = 0; i < arguments->key_payload_length; i++) {
        uint8_t r = 1 + (random() % 255);
        key[i] = r;           
    }    
    printf("random key generated.\n");
    uint32_t file_index = (random() % 1000);

    // Build packet
    pkt_request_t* pkt = pkt_request_new();
    create_pkt_request(pkt, file_index, arguments->key_size, (char *) key);
    char buf_request[arguments->key_payload_length + REQUEST_HEADER_LENGTH];
    pkt_request_encode(pkt, buf_request);
    
    // Send packet to server
    if (send(sockfd, buf_request, arguments->key_payload_length+REQUEST_HEADER_LENGTH, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

    // Read the response to create a packet, then delete it
    pkt_response_t *response_pkt = pkt_response_new();
    recv_response_packet(response_pkt, sockfd);
    pkt_response_del(response_pkt);

    close(sockfd);
    free(key);

    return NULL;
}

uint32_t get_gaussian_number(uint32_t mean, uint32_t std) {
    static uint32_t z2 = 0.0;
    uint32_t randn;
    if (z2 == 0.0) {
        uint32_t u1 = 2.0 * random() / RAND_MAX - 1;
        uint32_t u2 = 2.0 * random() / RAND_MAX - 1;
        uint32_t s = u1*u1 + u2*u2;
        while (s >= 1 || s == 0){
            u1 = 2.0 * random() / RAND_MAX - 1;
            u2 = 2.0 * random() / RAND_MAX - 1;
            s = u1*u1 + u2*u2;
        }
        uint32_t z1 = u1 * sqrt(-2.0 * log(s) / s);
        z2 = u2 * sqrt(-2.0 * log(s) / s);
        randn = (z1 * std) + mean;        
    } else {
        randn = (z2 * std) + mean;
        z2 = 0.0;     
    }
    return randn;
    
}