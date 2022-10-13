#include "../headers/packet_implem.h"



pkt_request_t* pkt_request_new()
{
    pkt_request_t* pkt;
    pkt = (pkt_request_t*) malloc(sizeof(pkt_request_t));
    if (pkt == NULL) return NULL;
    pkt->key = (char *) malloc(0); // To eliminate valgrind uninitialised error
    if (pkt->key == NULL) return NULL;
    return pkt;
}

pkt_response_t* pkt_response_new()
{
    pkt_response_t* pkt;
    pkt = (pkt_response_t*) malloc(sizeof(pkt_response_t));
    if (pkt == NULL) return NULL;
    pkt->encrypted_file = (char *) malloc(0); // To eliminate valgrind uninitialised error
    if (pkt->encrypted_file == NULL) return NULL;
    return pkt;
}

void pkt_request_del(pkt_request_t *pkt)
{
    free(pkt->key);
    pkt->key = NULL;
    free(pkt);
    pkt = NULL;
}

void pkt_response_del(pkt_response_t *pkt)
{
    free(pkt->encrypted_file);
    pkt->encrypted_file = NULL;
    free(pkt);
    pkt = NULL;
}

void pkt_request_decode(const char *data, pkt_request_t *pkt, bool header)
{
    if (header){
        pkt_request_set_findex(pkt, ntohl(((uint32_t *) (data))[0]));
        pkt_request_set_ksize(pkt, ntohl(((uint32_t *) (data))[1]));
    } else {
        uint64_t key_length = pkt_request_get_ksize(pkt) * pkt_request_get_ksize(pkt);
        pkt_request_set_key(pkt, data, key_length);
    }
    
}

void pkt_response_decode(const char *data, pkt_response_t *pkt, bool header)
{
    if (header) {
        pkt_response_set_errcode(pkt, ((uint8_t *) (data))[0]);
        pkt_response_set_fsize(pkt, ntohl(((uint32_t *) (data+1))[0]));
    } else {
        pkt_response_set_file(pkt, data, pkt_response_get_fsize(pkt));
    }
    
}

void pkt_request_encode(pkt_request_t* pkt, char *buf)
{
    uint32_t findex = htonl(pkt_request_get_findex(pkt));
    uint32_t ksize = htonl(pkt_request_get_ksize(pkt));
    for(int i=0; i<4; i++){
        buf[0+i] = (char) ((findex >> (8*i)) & 0xff);
    }
    for(int i=0; i<4; i++){
        buf[4+i] = (char) ((ksize >> (8*i)) & 0xff);
    }
    uint64_t key_length = pkt_request_get_ksize(pkt)*pkt_request_get_ksize(pkt);
    memcpy(buf+8, pkt_request_get_key(pkt), key_length);
}

void pkt_response_encode(pkt_response_t* pkt, char *buf)
{
    uint8_t errcode = (pkt_response_get_errcode(pkt));
    uint32_t fsize = htonl(pkt_response_get_fsize(pkt));
    buf[0] = (char) (errcode & 0xff);
    for(int i=0; i<4; i++){
        buf[1+i] = (char) ((fsize >> (8*i)) & 0xff);
    }
    memcpy(buf+5, pkt_response_get_file(pkt), pkt_response_get_fsize(pkt));
}


/* Getter and setter of response packets */
pkt_error_code pkt_response_get_errcode  (const pkt_response_t* pkt)
{
    return pkt->error_code;
}

pkt_status_code pkt_response_set_errcode(pkt_response_t *pkt, const pkt_error_code code)
{
    pkt->error_code = code;
    return PKT_OK;
}

uint32_t pkt_response_get_fsize  (const pkt_response_t* pkt)
{
    return pkt->file_size;
}

pkt_status_code pkt_response_set_fsize  (pkt_response_t* pkt, const uint32_t fsize)
{
    pkt->file_size = fsize;
    return PKT_OK;
}

const char* pkt_response_get_file(const pkt_response_t* pkt)
{
    return pkt->encrypted_file;
}

pkt_status_code pkt_response_set_file(pkt_response_t* pkt, const char *data, const uint32_t length)
{
    free(pkt->encrypted_file);
    pkt->encrypted_file = NULL;
    if (length > 0) {
        pkt->encrypted_file = (char*) malloc(length*sizeof(char));
        if (pkt->encrypted_file == NULL) return E_NOMEM;
        memcpy(pkt->encrypted_file, data, length);
    }
    return PKT_OK;
}




/* Getter and setter of request packets */
uint32_t pkt_request_get_findex  (const pkt_request_t* pkt)
{
    return pkt->file_index;
}

pkt_status_code pkt_request_set_findex  (pkt_request_t* pkt, const uint32_t findex)
{
    pkt->file_index = findex;
    return PKT_OK;
}

uint32_t pkt_request_get_ksize  (const pkt_request_t* pkt)
{
    return pkt->key_size;
}

pkt_status_code pkt_request_set_ksize  (pkt_request_t* pkt, const uint32_t ksize)
{
    pkt->key_size = ksize;
    return PKT_OK;
}

const char* pkt_request_get_key  (const pkt_request_t* pkt)
{
    return pkt->key;
}

pkt_status_code pkt_request_set_key(pkt_request_t* pkt, const char *data, const uint64_t length)
{
    free(pkt->key);
    pkt->key = NULL;
    if (length > 0) {
        pkt->key = (char*) malloc(length*sizeof(char));
        if (pkt->key == NULL) return E_NOMEM;
        memcpy(pkt->key, data, length);
    }
    return PKT_OK;
}

