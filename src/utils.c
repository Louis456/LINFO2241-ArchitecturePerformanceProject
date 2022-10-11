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

    pkt_request_decode(buf_header,pkt,1);
    uint32_t key_payload_length = pkt_request_get_ksize(pkt)*pkt_request_get_ksize(pkt);

    char buf_key[key_payload_length];
    if ((numbytes = recv(sockfd, buf_key, key_payload_length, 0)) == -1) {
        fprintf(stderr, "Error while receiving payload from client\n errno: %d\n", errno);
    }
    pkt_request_decode(buf_key,pkt,0);
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
    pkt_response_decode(buf_header,pkt,1);

    char buf_file[pkt_response_get_fsize(pkt)];
    if ((numbytes = recv(sockfd, buf_file, pkt_response_get_fsize(pkt), 0)) == -1) {
        fprintf(stderr, "Error while receiving file from server\n errno: %d\n", errno);
    }
    pkt_response_decode(buf_file,pkt,0);
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
    char *key = malloc(sizeof(char)*arguments->key_payload_length);       
    for (uint64_t i = 0; i < arguments->key_payload_length; i++) {
        char r = (char) (1 + (random() % 255));
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
    static double z2 = 0.0;
    double randn;
    if (z2 == 0.0) {
	double u1, u2, s;
        do {
            u1 = 2.0 * random() / RAND_MAX - 1;
            u2 = 2.0 * random() / RAND_MAX - 1;
            s = u1*u1 + u2*u2;
	} while (s >= 1 || s == 0);
        double z1 = u1 * sqrt(-2.0 * log(s) / s);
        z2 = u2 * sqrt(-2.0 * log(s) / s);
        randn = (z1 * std) + mean;        
    } else {
        randn = (z2 * std) + mean;
        z2 = 0.0;     
    }
    return (uint32_t) randn;
    
}


bool isEmpty(request_queue_t* queue) 
{
    return queue->size == 0;
}


void push(request_queue_t* queue, int sockfd, pkt_request_t* pkt)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node == NULL) fprintf(stderr, "Error malloc a new queue node\n");
    
    new_node->fd = sockfd;
    new_node->pkt = pkt;
    new_node->next = NULL;
    
    if (isEmpty(queue)) {
        queue->head = new_node;
        queue->tail = new_node;
        queue->size = 1;
    } else {
        queue->tail->next = new_node;
        queue->tail = new_node;
        queue->size += 1;
    }
}
 
node_t* pop(request_queue_t* queue)
{
    if (isEmpty(queue)) fprintf(stderr, "cannot pop an empty queue");

    node_t *head = queue->head;
    queue->head = queue->head->next;
    queue->size -= 1;
    return head;
}

void encrypt_file(char *encrypted_file, char **file, uint32_t file_size, char *key, uint32_t key_size) {
    // Travel by sub squares of key size
    uint32_t file_div_key = file_size / key_size;
    for (uint32_t l = 0; l < file_div_key; l++) {
        for (uint32_t m = 0; m < file_div_key; m++) { 
            for (uint32_t i = 0; i < key_size; i++) {
                for (uint32_t j = 0; j < key_size; j++) { 
                    encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] = 0;
                    for(uint32_t k = 0; k < key_size; k++) {
                        encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k] * file[l * key_size + k][m * key_size + j];
                    }
                }
            }
        }
    }
}

void* start_server_thread(void* args) {
    server_thread_args *arguments = (server_thread_args *) args;

    // Retrieve request values
    pkt_request_t *pkt_request = arguments->pkt;
    uint32_t file_index = pkt_request->file_index;
    uint32_t key_size = pkt_request->key_size;
    char* key = pkt_request->key;

    // Get file and encrypt it
    char **file = (arguments->files)[file_index];
    uint32_t fsize = arguments->fsize;
    char* encrypted_file = malloc(sizeof(char) * fsize * fsize);
    encrypt_file(encrypted_file, file, fsize, key, key_size); 
    pkt_request_del(pkt_request);
    
    // Create response packet
    pkt_response_t* pkt_response = pkt_response_new();
    uint8_t code = 0;
    create_pkt_response(pkt_response, code, fsize, encrypted_file);

    // Encode buffer and send it
    uint32_t total_size = fsize + RESPONSE_HEADER_LENGTH;
    char* buf = malloc(sizeof(char) * total_size);
    pkt_response_encode(pkt_response, buf);
    if (send(arguments->fd, buf, total_size, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

    // Free and close
    pkt_response_del(pkt_response);
    close(arguments->fd);
    free(encrypted_file);
    free(buf);
    *(arguments->status) = STOPPED;
    return NULL;
}
