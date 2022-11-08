#include "../headers/threads.h"



void* start_server_thread(void* args) {
    // Retrieve arguments
    server_thread_args *arguments = (server_thread_args *) args;
    uint32_t fsize = arguments->fsize;
    
    // Read client request
    pkt_request_t* pkt_request = pkt_request_new();
    if (pkt_request == NULL) fprintf(stderr, "Error while making a new request packet in server thread\n");
    if (recv_request_packet(pkt_request, arguments->fd, fsize) != PKT_OK) {
        // Invalid key size
        /*
        fprintf(stderr, "Key size must divide the file size\n");
        pkt_response_t* pkt_response = pkt_response_new();
        if (pkt_response== NULL) fprintf(stderr, "Error while making a new response packet in start_server_thread\n");
        char error_message[] = "invalid key size";
        uint8_t error_message_size = strlen(error_message)+1;
        uint8_t error_code = 1;
        create_pkt_response(pkt_response, error_code, error_message_size, error_message);
        u_int8_t total_size = RESPONSE_HEADER_LENGTH + error_message_size;
        char* buf = malloc(sizeof(char) * total_size);
        if (buf == NULL) fprintf(stderr, "Error malloc: buf response invalid key\n");
        if (send(arguments->fd, buf, total_size, 0) == -1) fprintf(stderr, "send failed with invalid key size\n errno: %d\n", errno);
        free(buf);
        pkt_response_del(pkt_response);
        */
        return NULL;
    }
    uint32_t file_index = pkt_request->file_index;
    uint32_t key_size = pkt_request->key_size;
    uint32_t* key = (uint32_t *) pkt_request->key;

    // Get file and encrypt it
    uint32_t *file = &((arguments->files)[file_index * fsize * fsize]);
    uint32_t* encrypted_file = malloc(sizeof(uint32_t) * fsize * fsize);
    if (encrypted_file == NULL) fprintf(stderr, "Error malloc: encrypted_file\n");
    encrypt_file(encrypted_file, file, fsize, key, key_size, NOT_OPTI);
    pkt_request_del(pkt_request);
    
    // Create response packet
    pkt_response_t* pkt_response = pkt_response_new();
    if (pkt_response== NULL) fprintf(stderr, "Error while making a new response packet in start_server_thread\n");
    uint8_t code = 0;
    create_pkt_response(pkt_response, code, fsize*fsize, encrypted_file);

    // Encode buffer and send it
    uint32_t total_size = (fsize * fsize * sizeof(uint32_t)) + RESPONSE_HEADER_LENGTH;
    char* buf = malloc(sizeof(char) * total_size);
    if (buf == NULL) fprintf(stderr, "Error malloc: buf response\n");
    pkt_response_encode(pkt_response, buf);
    if (send(arguments->fd, buf, total_size, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

    // Free and close
    pkt_response_del(pkt_response);
    close(arguments->fd);
    free(encrypted_file);
    free(buf);
    *(arguments->status) = STOPPED;
    free(arguments);
    pthread_exit(0);
}



void* start_client_thread(void* args) {
    client_thread_args *arguments = (client_thread_args *) args;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);
    
    if (connect(sockfd, (struct sockaddr*) arguments->servaddr, sizeof(*arguments->servaddr)) == -1) {
        close(sockfd);
        fprintf(stderr, "Error while connecting to server\n errno: %d\n", errno);
    }
    
    // Generate random key and file index
    uint32_t *key = malloc(sizeof(uint32_t) * arguments->key_payload_length);   
    if (key == NULL) fprintf(stderr, "Error malloc: key payload\n");  
    uint32_t file_index = (random() % 1000);
    // Build packet
    pkt_request_t* pkt = pkt_request_new();
    if (pkt == NULL) fprintf(stderr, "Error while making a new request packet in start_client\n");
    create_pkt_request(pkt, file_index, arguments->key_size, key);
    uint32_t buf_size = arguments->key_payload_length * sizeof(uint32_t) + REQUEST_HEADER_LENGTH;
    char buf_request[buf_size];
    pkt_request_encode(pkt, buf_request);
    pkt_request_del(pkt);
    
    // Send packet to server
    if (send(sockfd, buf_request, buf_size, 0) == -1) fprintf(stderr, "send failed\n errno: %d\n", errno);

    // Start timer
    /*
    struct timeval start_at;
    get_current_clock(&start_at);
    */

    // Read the response to create a packet, then delete it
    pkt_response_t *response_pkt = pkt_response_new();
    if (response_pkt== NULL) fprintf(stderr, "Error while making a new response packet in start_client\n");
    recv_response_packet(response_pkt, sockfd);

    //*(arguments->bytes_sent_rcvd) = arguments->key_payload_length*sizeof(uint32_t) + REQUEST_HEADER_LENGTH + RESPONSE_HEADER_LENGTH + (pkt_response_get_fsize(response_pkt) * sizeof(uint32_t));

    // Stop timer and store elapsed time in response_time
    /*
    struct timeval end_at;
    get_current_clock(&end_at);
    struct timeval diff_time;
    timersub(&end_at, &start_at, &diff_time);
    *(arguments->response_time) = get_us(&diff_time);
    */

    pkt_response_del(response_pkt);

    close(sockfd);
    free(key);
    free(arguments);
    pthread_exit(0);
}



void encrypt_file(uint32_t *encrypted_file, uint32_t *file, uint32_t file_size, uint32_t *key, uint32_t key_size, opti_choice opti) {
    if (opti == NOT_OPTI) {
        // Travel by sub squares of key size
        uint32_t file_div_key = file_size / key_size;
        for (uint32_t l = 0; l < file_div_key; l++) {
            for (uint32_t m = 0; m < file_div_key; m++) { 
                for (uint32_t i = 0; i < key_size; i++) {
                    for (uint32_t j = 0; j < key_size; j++) { 
                        encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] = 0;
                        for(uint32_t k = 0; k < key_size; k++) {
                            encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                        }
                    }
                }
            }
        }
    } else if (opti == INLINING) {
        for (uint32_t i = 0; i < file_size; i++) {
            uint32_t key_block = (i % key_size) * key_size;
            for(uint32_t j = 0; j < file_size; j++) {
                encrypted_file[i*file_size + j] = key[key_block] * file[(i-(i%key_size))*file_size + j];
            }
            for (uint32_t k = 1; k < key_size; k++) {
                uint32_t r = key[key_block + k];
                for(uint32_t j = 0; j < file_size; j++) {
                    encrypted_file[i*file_size + j] += r * file[(i-(i%key_size)+k)*file_size+j];
                }
            }
        }
    } else if (opti == LOOP_UNROLLING) {
        uint32_t file_div_key = file_size / key_size;
        for (uint32_t l = 0; l < file_div_key; l++) {
            for (uint32_t m = 0; m < file_div_key; m++) { 
                for (uint32_t i = 0; i < key_size; i++) {
                    for (uint32_t j = 0; j < key_size; j++) { 
                        encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] = 0;
                        if (key_size >= 8) {
                            for(uint32_t k = 0; k < key_size; k+=8) {                    
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+1] * file[((l * key_size + k+1) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+2] * file[((l * key_size + k+2) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+3] * file[((l * key_size + k+3) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+4] * file[((l * key_size + k+4) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+5] * file[((l * key_size + k+5) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+6] * file[((l * key_size + k+6) * file_size) + (m * key_size + j)];
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k+7] * file[((l * key_size + k+7) * file_size) + (m * key_size + j)];
                            }
                        } else {
                            for(uint32_t k = 0; k < key_size; k++) {
                                encrypted_file[((l * key_size + i) * file_size) + (m * key_size + j)] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                            }  
                        }
                    }
                }
            }
        }
    } else if (opti == BOTH_OPTI) {
        for (uint32_t i = 0; i < file_size; i++) {
            uint32_t key_block = (i % key_size) * key_size;
            for(uint32_t j = 0; j < file_size; j+=8) {
                encrypted_file[i*file_size + j] = key[key_block] * file[(i-(i%key_size))*file_size + j];
                encrypted_file[i*file_size + j+1] = key[key_block] * file[(i-(i%key_size))*file_size + j+1];
                encrypted_file[i*file_size + j+2] = key[key_block] * file[(i-(i%key_size))*file_size + j+2];
                encrypted_file[i*file_size + j+3] = key[key_block] * file[(i-(i%key_size))*file_size + j+3];
                encrypted_file[i*file_size + j+4] = key[key_block] * file[(i-(i%key_size))*file_size + j+4];
                encrypted_file[i*file_size + j+5] = key[key_block] * file[(i-(i%key_size))*file_size + j+5];
                encrypted_file[i*file_size + j+6] = key[key_block] * file[(i-(i%key_size))*file_size + j+6];
                encrypted_file[i*file_size + j+7] = key[key_block] * file[(i-(i%key_size))*file_size + j+7];
            }
            for (uint32_t k = 1; k < key_size; k++) {
                uint32_t r = key[key_block + k];
                for(uint32_t j = 0; j < file_size; j+=8) {
                    encrypted_file[i*file_size + j] += r * file[(i-(i%key_size)+k)*file_size+j];
                    encrypted_file[i*file_size + j+1] += r * file[(i-(i%key_size)+k)*file_size+j+1];
                    encrypted_file[i*file_size + j+2] += r * file[(i-(i%key_size)+k)*file_size+j+2];
                    encrypted_file[i*file_size + j+3] += r * file[(i-(i%key_size)+k)*file_size+j+3];
                    encrypted_file[i*file_size + j+4] += r * file[(i-(i%key_size)+k)*file_size+j+4];
                    encrypted_file[i*file_size + j+5] += r * file[(i-(i%key_size)+k)*file_size+j+5];
                    encrypted_file[i*file_size + j+6] += r * file[(i-(i%key_size)+k)*file_size+j+6];
                    encrypted_file[i*file_size + j+7] += r * file[(i-(i%key_size)+k)*file_size+j+7];
                }
            }
        }
    }
}
