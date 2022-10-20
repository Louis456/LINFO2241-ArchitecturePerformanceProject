#ifndef __UTILS_H_
#define __UTILS_H_

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */
#include <unistd.h>
#include <stdio.h>  /* ssize_t */
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

#include "packet_implem.h"

typedef struct node_t node_t;

typedef struct node_t {
  int fd;
  node_t *next;
} node_t;

typedef struct {
  node_t *head;
  node_t *tail;
  int size;
} request_queue_t;





/* Fill the pkt_response structure from the given parameters */
void create_pkt_response(pkt_response_t* pkt, pkt_error_code code, uint32_t fsize, char* file);

/* Fill the pkt_request structure from the given parameters */
void create_pkt_request(pkt_request_t* pkt, uint32_t findex, uint32_t ksize, char *key);

/* Read from sockfd to create a response packet */
pkt_status_code recv_request_packet(pkt_request_t* pkt, int sockfd, uint32_t file_size);


/* Read from sockfd to create a response_packet */
pkt_status_code recv_response_packet(pkt_response_t* pkt, int sockfd);

/* Fill the timeval structure with the current clock time */
void get_current_clock(struct timeval *timestamp);

/* Return ms from timeval structure */
uint64_t get_ms(struct timeval *timestamp);

/* Return us from timeval structure */
uint64_t get_us(struct timeval *timestamp);


/* Get random number following a normal distribution from paramter's mean and std. 
 * Uses the Box-Muller algorithm 
 * @pre: rand should be initialised, e.g. using: srand(time(NULL)); */
uint32_t get_gaussian_number(double mean, double std);


/* Get random number following an exponantial distribution with the parameter rate 
 * @pre: lambda or the rate 
 * @return: the generated number
*/
double get_exponential_number(double rate);

/* Check if the queue of requests is empty
 * @pre: request_queue_t, the queue of requests
 * @return: True if the queue is empty
 *          else False 
 */
bool isEmpty(request_queue_t* queue) ;

/* Push a node with the fd to receive a request from in the queue
 * @pre: request_queue_t, the queue to push in
 * @pre: fd, a file descriptor
 */
void push(request_queue_t* queue, int sockfd);

/* Pop a node with a fd to receive from from the queue
 * @pre: request_queue_t, the queue to pop from
 * @return: the node_t containing the fd
 */
node_t* pop(request_queue_t* queue);

/* Return the sum of values in values
 * @pre: values the array of values 
 *       the length of the array
 * @return: the sum as a uint
*/ 
uint32_t get_sum(uint32_t *values, uint32_t length);

uint32_t get_mean(uint32_t *values, uint32_t length);

uint32_t get_variance(uint32_t *values, uint32_t length);

uint32_t get_std(uint32_t *values, uint32_t length);

double get_sum_double(uint32_t *values, uint32_t length);

double get_mean_double(uint32_t *values, uint32_t length);

double get_variance_double(uint32_t *values, uint32_t length);

double get_std_double(uint32_t *values, uint32_t length);

#endif  /* __UTILS_H_ */
