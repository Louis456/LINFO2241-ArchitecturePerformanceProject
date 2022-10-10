#ifndef __PACKET_IMPLEM_H_
#define __PACKET_IMPLEM_H_

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */
#include <unistd.h>
#include <stdio.h>  /* ssize_t */
#include <zlib.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

typedef struct pkt_request pkt_request_t;
typedef struct pkt_response pkt_response_t;

#define RESPONSE_HEADER_LENGTH 5
#define MAX_RESPONSE_LENGTH 1048576
#define REQUEST_HEADER_LENGTH 8
#define MAX_REQUEST_LENGTH 1048576

enum ErrorCode
{
    OK = 0,
    FAILED = 1
};
typedef uint8_t pkt_error_code;

typedef enum {
	PKT_OK = 0,     /* Success */
	E_NOMEM,        /* Not enough memory */
} pkt_status_code;

/* Initialize a new struct pkt_request
 * @return: NULL if something went wrong */
pkt_request_t* pkt_request_new();

/* Initialize a new struct pkt_response
 * @return: NULL if something went wrong */
pkt_response_t* pkt_response_new();

/* Free the structure and its associated ressources */
void pkt_request_del(pkt_request_t *pkt);

/* Free the structure and its associated ressources */
void pkt_response_del(pkt_response_t *pkt);

/* Decode the data from the buffer into a pkt_request structure.
 * @data: array of bytes 
 * @len: number of received bytes
 * @pkt: a valid pkt_request_t struct in which the data will be stored
*/
void pkt_request_decode(const char *data, pkt_request_t *pkt);

/* Decode the data from the buffer into a pkt_response structure.
 * @data: array of bytes 
 * @len: number of received bytes
 * @pkt: a valid pkt_response_t struct in which the data will be stored
*/
void pkt_response_decode(const char *data, pkt_response_t *pkt);

/* Encode a pkt_request into a buffer, in network-byte-order.
 * @pkt: structure to be decoded
 * @buf: array of bytes to store to be encoded from the structure with enough memory
*/
void pkt_request_encode(const pkt_request_t*, char *buf);

/* Encode a pkt_response into a buffer, in network-byte-order.
 * @pkt: structure to be decoded
 * @buf: array of bytes to store to be encoded from the structure with enough memory
*/
void pkt_response_encode(const pkt_response_t*, char *buf);


/* Getter and setter of response packets */
pkt_error_code pkt_response_get_errcode(const pkt_response_t* pkt);
pkt_status_code pkt_response_set_errcode(pkt_response_t *pkt, const pkt_error_code code);
uint32_t pkt_response_get_fsize(const pkt_response_t* pkt);
pkt_status_code pkt_response_set_fsize(pkt_response_t* pkt, const uint32_t fsize);
const char* pkt_response_get_file(const pkt_response_t* pkt);
pkt_status_code pkt_response_set_file(pkt_response_t* pkt, const char *data, const uint32_t length);


/* Getter and setter of request packets */
uint32_t pkt_request_get_findex(const pkt_request_t* pkt);
pkt_status_code pkt_request_set_findex(pkt_request_t* pkt, const uint32_t findex);
uint32_t pkt_request_get_ksize(const pkt_request_t* pkt);
pkt_status_code pkt_request_set_ksize(pkt_request_t* pkt, const uint32_t ksize);
const char* pkt_request_get_key(const pkt_request_t* pkt);
pkt_status_code pkt_request_set_key(pkt_request_t* pkt, const char *data, const uint64_t length);

#endif  /* __PACKET_IMPLEM_H_ */