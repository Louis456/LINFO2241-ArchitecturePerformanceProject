#include "../headers/utils.h"


void create_pkt_response(pkt_response_t* pkt, pkt_error_code code, uint32_t fsize, uint32_t* file)
{
    pkt_status_code status_code = pkt_response_set_errcode(pkt, code);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the error code in response packet");

    status_code = pkt_response_set_fsize(pkt, fsize);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file size in response packet");

    status_code = pkt_response_set_file(pkt, (char *) file, fsize * sizeof(uint32_t));
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file payload in response packet");
}

pkt_status_code recv_request_packet(pkt_request_t* pkt, int sockfd, uint32_t file_size) {
    //printf("Receive packet\n");

    uint32_t fileid;
    int numbytes;
    //printf("Receive fileid\n");
    if ((numbytes = recv(sockfd, &fileid, 4, 0)) == -1) {
        fprintf(stderr, "Error while receiving header from client\n errno: %d\n", errno);
    }

    uint32_t keysize;
    //printf("Receive keysize\n");
    if ((numbytes = recv(sockfd, &keysize, 4, 0)) == -1) {
        fprintf(stderr, "Error while receiving header from client\n errno: %d\n", errno);
    }

    //printf("Decode\n");
    char buf_header[REQUEST_HEADER_LENGTH];
    ((uint32_t *) buf_header)[0] = fileid;
    ((uint32_t *) buf_header)[1] = keysize;
    pkt_request_decode(buf_header, pkt, true);
    
    if (file_size % pkt_request_get_ksize(pkt) != 0) {
        printf("Inside E_KEY_SIZE, %d, %d\n", file_size, keysize);
        return E_KEY_SIZE;
    }

    uint32_t key_payload_length = pkt_request_get_ksize(pkt)*pkt_request_get_ksize(pkt)*sizeof(uint32_t);

    char buf_key[key_payload_length];
    uint32_t tot = key_payload_length;
    uint32_t done = 0;
    //printf("While receive buffer\n");
    while (done < tot) {
        //printf("in while\n");
        if ((numbytes = recv(sockfd, buf_key, tot - done, 0)) == -1) {
            fprintf(stderr, "Error while receiving payload from client\n errno: %d\n", errno);
        }
        done += numbytes;
    }
    //printf("Decode\n");
    pkt_request_decode(buf_key, pkt, false);
    //printf("Key: %d\n", pkt_request_get_findex(pkt));
    //printf("Receive DONE\n");
    return PKT_OK;
}

void create_pkt_request(pkt_request_t* pkt, uint32_t findex, uint32_t ksize, uint32_t *key)
{
    pkt_status_code status_code = pkt_request_set_findex(pkt, findex);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the file index in request packet");

    //printf("key_size create_pkt_request: %d\n", ksize);
    status_code = pkt_request_set_ksize(pkt, ksize);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the key size in request packet");

    uint64_t key_payload_length = ksize * ksize * sizeof(uint32_t);
    status_code = pkt_request_set_key(pkt, (char *) key, key_payload_length);
    if (status_code != PKT_OK) fprintf(stderr, "error setting the key in request packet");
}

pkt_status_code recv_response_packet(pkt_response_t* pkt, int sockfd) {
    char buf_header[RESPONSE_HEADER_LENGTH];
    int numbytes;
    if ((numbytes = recv(sockfd, buf_header, RESPONSE_HEADER_LENGTH, 0)) == -1) {
        fprintf(stderr, "Error while receiving header from server\n errno: %d\n", errno);
    }
    pkt_response_decode(buf_header, pkt, true);

    char buf_file[pkt_response_get_fsize(pkt) * sizeof(uint32_t)];
    if ((numbytes = recv(sockfd, buf_file, pkt_response_get_fsize(pkt) * sizeof(uint32_t), 0)) == -1) {
        fprintf(stderr, "Error while receiving file from server\n errno: %d\n", errno);
    }
    pkt_response_decode(buf_file, pkt, false);
    return PKT_OK;
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



uint32_t get_gaussian_number(double mean, double std) {
    static double z2 = 0.0;
    double randn;
    if (z2 == 0.0) {
        double u1, u2, s;
        do {
            u1 = 2.0 * (((double) random()) / RAND_MAX) - 1;
            u2 = 2.0 * (((double) random()) / RAND_MAX) - 1;
            s = u1*u1 + u2*u2;
        } while (s >= 1.0 || s == 0.0);
        double w = sqrt((-2.0 * log(s)) / s);
        double z1 = u1 * w;
        z2 = u2 * w;
        randn = (z1 * std) + mean;        
    } else {
        randn = (z2 * std) + mean;
        z2 = 0.0;     
    }
    if (randn < 0) return 0;
    return (uint32_t) randn; 
}

double get_exponential_number(double rate) {
    double U;
    do {
        U = 1.0f - (double) random() / (RAND_MAX);
    } while (U == 0);
    return -logf(1.0f - U) / (rate/1000000);
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

