#include "../headers/utils.h"

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

