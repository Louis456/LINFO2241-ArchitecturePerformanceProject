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

/* Fill the timeval structure with the current clock time */
void get_current_clock(struct timeval *timestamp);

/* Return ms from timeval structure */
uint64_t get_ms(struct timeval *timestamp);

/* Return us from timeval structure */
uint64_t get_us(struct timeval *timestamp);

#endif  /* __UTILS_H_ */
