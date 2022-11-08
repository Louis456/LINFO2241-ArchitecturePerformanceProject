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

typedef enum {
	NOT_OPTI = 0,
	INLINING,
  LOOP_UNROLLING,
  BOTH_OPTI
} opti_choice;


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
 * @pre: values the array of positive values 
 *       the length of the array
 * @return: the sum as a uint
*/ 
uint32_t get_sum(uint32_t *values, uint32_t length);


/* Return the mean of the values in the array
 * @pre: values the array of positive values
 *       the length of the array
 * @return: the mean as a uint
*/
uint32_t get_mean(uint32_t *values, uint32_t length);


/* Return the variance of the values in the array
 * @pre: values the array of positive values
 *       the length of the array
 * @return: the variance as a uint
*/
uint32_t get_variance(uint32_t *values, uint32_t length);


/* Return the std of the values in the array
 * @pre: values the array of positive values
 *       the length of the array
 * @return: the std as a uint
*/
uint32_t get_std(uint32_t *values, uint32_t length);

// The following function are the same as above but for double values
double get_sum_double(uint32_t *values, uint32_t length);

double get_mean_double(uint32_t *values, uint32_t length);

double get_variance_double(uint32_t *values, uint32_t length);

double get_std_double(uint32_t *values, uint32_t length);

#endif  /* __UTILS_H_ */
