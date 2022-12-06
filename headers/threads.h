#ifndef __THREADS_H_
#define __THREADS_H_

#include <inttypes.h> /* uintx_t */
#include <stddef.h> /* size_t */
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
#include <arpa/inet.h>
#include <immintrin.h>
#include <x86intrin.h>

#include "utils.h"

typedef enum {
	STOPPED = 0,
	RUNNING
} thread_status_code;

typedef struct {
  struct sockaddr_in *servaddr;
  uint64_t key_payload_length;
  uint32_t key_size;
} client_thread_args;

/* Take a file as a matrix and divide it into sub-squares of size key_size*key_size
 * Then applies matrix multiplication between those sub_squares and the key.
 * @pre: file of size file_size*file_size
 * @pre: key of size key_size*key_size
 * @return: fills encrypted_file the result of the matrix multiplications of each sub-squareconcatenated in a single array
 * 
*/
void encrypt_file(float *encrypted_file, float *file, uint32_t file_size, float *key, uint32_t key_size);

/* Funtion called by client main to start a thread which will establish a connection with the server, generate a key, send a request and wait for a response 
 * @pre: key size and server information
*/
void* start_client_thread(void* args);

#endif  /* __UTILS_H_ */