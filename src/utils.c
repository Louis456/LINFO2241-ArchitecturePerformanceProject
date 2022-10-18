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

pkt_status_code recv_request_packet(pkt_request_t* pkt, int sockfd, uint32_t file_size) {
    char buf_header[REQUEST_HEADER_LENGTH+1];
    int numbytes;
    if ((numbytes = recv(sockfd, buf_header, REQUEST_HEADER_LENGTH, 0)) == -1) {
        fprintf(stderr, "Error while receiving header from client\n errno: %d\n", errno);
    }

    pkt_request_decode(buf_header,pkt,true);
    if (file_size % pkt_request_get_ksize(pkt) != 0) {
        return E_KEY_SIZE;
    }
    uint32_t key_payload_length = pkt_request_get_ksize(pkt)*pkt_request_get_ksize(pkt);

    char buf_key[key_payload_length];
    if ((numbytes = recv(sockfd, buf_key, key_payload_length, 0)) == -1) {
        fprintf(stderr, "Error while receiving payload from client\n errno: %d\n", errno);
    }
    pkt_request_decode(buf_key,pkt,false);
    return PKT_OK;
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
    pkt_response_decode(buf_header,pkt,true);

    char buf_file[pkt_response_get_fsize(pkt)];
    if ((numbytes = recv(sockfd, buf_file, pkt_response_get_fsize(pkt), 0)) == -1) {
        fprintf(stderr, "Error while receiving file from server\n errno: %d\n", errno);
    }
    pkt_response_decode(buf_file,pkt,false);
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

uint64_t get_us(struct timeval *timestamp) {
    return timestamp->tv_sec * 1000000 + timestamp->tv_usec;
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
    //printf("Start generating a random key\n");
    char *key = malloc(sizeof(char)*arguments->key_payload_length);   
    if (key == NULL) fprintf(stderr, "Error malloc: key payload\n");
    for (uint64_t i = 0; i < arguments->key_payload_length; i++) {
        char r = (char) (1 + (random() % 255));
        key[i] = r;           
    }    
    //printf("random key generated.\n");
    uint32_t file_index = (random() % 1000);

    // Build packet
    pkt_request_t* pkt = pkt_request_new();
    if (pkt == NULL) fprintf(stderr, "Error while making a new request packet in start_client\n");
    create_pkt_request(pkt, file_index, arguments->key_size, (char *) key);
    char buf_request[arguments->key_payload_length + REQUEST_HEADER_LENGTH];
    pkt_request_encode(pkt, buf_request);
    
    // Send packet to server
    if (send(sockfd, buf_request, arguments->key_payload_length+REQUEST_HEADER_LENGTH, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

    // Start timer
    struct timeval start_at;
    get_current_clock(&start_at);

    // Read the response to create a packet, then delete it
    pkt_response_t *response_pkt = pkt_response_new();
    if (response_pkt== NULL) fprintf(stderr, "Error while making a new response packet in start_client\n");
    recv_response_packet(response_pkt, sockfd);
    //printf("received encrypted file number : %d of size %d\n",file_index,pkt_response_get_fsize(response_pkt));

    *(arguments->bytes_sent_rcvd) = arguments->key_payload_length + REQUEST_HEADER_LENGTH + RESPONSE_HEADER_LENGTH + pkt_response_get_fsize(response_pkt);

    // Stop timer and store elapsed time in response_time
    struct timeval end_at;
    get_current_clock(&end_at);
    struct timeval diff_time;
    timersub(&end_at, &start_at, &diff_time);
    *(arguments->response_time) = get_ms(&diff_time);

    pkt_response_del(response_pkt);

    //printf("before closing socket\n");
    close(sockfd);
    //printf("after closing socket\n");
    free(key);
    free(arguments);

    return NULL;
}

uint32_t get_gaussian_number(double mean, double std) {
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


void push(request_queue_t* queue, int sockfd)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node == NULL) fprintf(stderr, "Error malloc a new queue node\n");
    
    new_node->fd = sockfd;
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
    // Retrieve arguments
    server_thread_args *arguments = (server_thread_args *) args;
    uint32_t fsize = arguments->fsize;
    
    // Read client request
    printf("file descriptor beginning in thread[%d] : %d\n", arguments->id,arguments->fd);
    pkt_request_t* pkt_request = pkt_request_new();
    if (pkt_request == NULL) fprintf(stderr, "Error while making a new request packet in server thread\n");
    if (recv_request_packet(pkt_request, arguments->fd, fsize) != PKT_OK) {
        // Invalid key size
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
        return NULL;
    }
    uint32_t file_index = pkt_request->file_index;
    uint32_t key_size = pkt_request->key_size;
    char* key = pkt_request->key;

    // Get file and encrypt it
    char **file = (arguments->files)[file_index];
    char* encrypted_file = malloc(sizeof(char) * fsize * fsize);
    if (encrypted_file == NULL) fprintf(stderr, "Error malloc: encrypted_file\n");
    encrypt_file(encrypted_file, file, fsize, key, key_size); 
    pkt_request_del(pkt_request);
    
    // Create response packet
    pkt_response_t* pkt_response = pkt_response_new();
    if (pkt_response== NULL) fprintf(stderr, "Error while making a new response packet in start_server_thread\n");
    uint8_t code = 0;
    create_pkt_response(pkt_response, code, fsize*fsize, encrypted_file);

    // Encode buffer and send it
    uint32_t total_size = fsize*fsize + RESPONSE_HEADER_LENGTH;
    char* buf = malloc(sizeof(char) * total_size);
    if (buf == NULL) fprintf(stderr, "Error malloc: buf response\n");
    pkt_response_encode(pkt_response, buf);
    printf("file descriptor end in thread[%d] : %d\n", arguments->id,arguments->fd);
    if (send(arguments->fd, buf, total_size, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

    // Free and close
    pkt_response_del(pkt_response);
    close(arguments->fd);
    free(encrypted_file);
    free(buf);
    //printf("Thread %d STOP\n", arguments->id);
    *(arguments->status) = STOPPED;
    return NULL;
}

uint32_t get_sum(uint32_t *values, uint32_t length) {
    if (length == 0 || values == NULL) return 0;   
    uint32_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {        
        sum += values[i];
    }
    return sum;
}

double get_sum_double(uint32_t *values, uint32_t length) {
    if (length == 0 || values == NULL) return 0;   
    double sum = 0;
    for (uint32_t i = 0; i < length; i++) {        
        sum += values[i];
    }
    return sum;
}

uint32_t get_mean(uint32_t *values, uint32_t length) {
    if (length == 0 || values == NULL) return 0;
    uint32_t sum = get_sum(values, length);
    return sum/length;
}

double get_mean_double(uint32_t *values, uint32_t length) {
    if (length == 0 || values == NULL) return 0;
    double sum = get_sum_double(values, length);
    return sum/length;
}

uint32_t get_variance(uint32_t *values, uint32_t length) {
    if (length == 0 || values == NULL) return 0;
    uint32_t mean = get_mean(values, length);
    uint32_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += pow((values[i] - mean), 2);
    }
    return sum / length;
}

double get_variance_double(uint32_t *values, uint32_t length) {
    if (length == 0 || values == NULL) return 0;
    double mean = get_mean_double(values, length);
    double sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += pow((values[i] - mean), 2);
    }
    return sum / length;
}

uint32_t get_std(uint32_t *values, uint32_t length) {
    return sqrt(get_variance(values, length));
}

double get_std_double(uint32_t *values, uint32_t length) {
    return sqrt(get_variance_double(values, length));
}

