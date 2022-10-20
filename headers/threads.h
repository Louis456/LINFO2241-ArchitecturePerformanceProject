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
  char ***files;
} server_thread_args;

void encrypt_file(char *encrypted_file, char **file, uint32_t file_size, char *key, uint32_t key_size);

void* start_server_thread(void* args);

void* start_client_thread(void* args);

#endif  /* __UTILS_H_ */