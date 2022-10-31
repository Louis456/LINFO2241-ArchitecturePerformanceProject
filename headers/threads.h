#ifndef __THREADS_H_
#define __THREADS_H_

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

#include "utils.h"

typedef enum {
	STOPPED = 0,
	RUNNING
} thread_status_code;

typedef struct {
  struct addrinfo *serverinfo;
  uint64_t key_payload_length;
  uint32_t key_size;
  uint32_t *response_time;
  uint32_t *bytes_sent_rcvd;
} client_thread_args;

typedef struct {
  uint8_t id;
  int fd;
  pkt_request_t *pkt;
  thread_status_code *status;
  uint32_t fsize;
  uint32_t *files;
} server_thread_args;

/* Take a file as a matrix and divide it into sub-squares of size key_size*key_size
 * Then applies matrix multiplication between those sub_squares and the key.
 * @pre: file of size file_size*file_size
 * @pre: key of size key_size*key_size
 * @return: fills encrypted_file the result of the matrix multiplications of each sub-squareconcatenated in a single array
 * 
*/
void encrypt_file(uint32_t *encrypted_file, uint32_t *file, uint32_t file_size, uint32_t *key, uint32_t key_size);

/* Function called by the server main to listen on a file descriptor, receive a request, encrypt a file, and send back a response 
 * @pre: structure server_thread_args containing the file descriptor
 * @pre: all the files to pick one with the index received in the request
 * @pre: file size
 * @return: sends back a response with the encrypted file
*/ 
void* start_server_thread(void* args);

/* Funtion called by client main to start a thread which will establish a connection with the server, generate a key, send a request and wait for a response 
 * @pre: key size and server information
*/
void* start_client_thread(void* args);

#endif  /* __UTILS_H_ */