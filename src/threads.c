#include "../headers/threads.h"

void* start_client_thread(void* args) {
    client_thread_args *arguments = (client_thread_args *) args;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) fprintf(stderr, "Error while creating the socket\n errno: %d\n", errno);
    
    if (connect(sockfd, (struct sockaddr*) arguments->servaddr, sizeof(*arguments->servaddr)) == -1) {
        close(sockfd);
        fprintf(stderr, "Error while connecting to server\n errno: %d\n", errno);
    }
    
    uint32_t *key = malloc(sizeof(uint32_t) * arguments->key_payload_length);   
    if (key == NULL) fprintf(stderr, "Error malloc: key payload\n");

    // Send request
    uint32_t file_index = htonl(random() % 1000);
    uint32_t key_size = htonl(arguments->key_size);
    if (send(sockfd, &file_index, 4, 0) == -1) fprintf(stderr, "send failed, request fileindex\n errno: %d\n", errno);
    if (send(sockfd, &key_size, 4, 0) == -1) fprintf(stderr, "send failed, request key_size\n errno: %d\n", errno);
    if (send(sockfd, key, sizeof(uint32_t) * arguments->key_payload_length, 0) == -1) fprintf(stderr, "send failed, request key\n errno: %d\n", errno);

    // Start timer
    struct timeval start_at;
    get_current_clock(&start_at);

    // Read the response
    int numbytes;
    uint8_t error; 
    if ((numbytes = recv(sockfd, &error, 1, 0)) == -1) 
        fprintf(stderr, "Error while receiving error code\n errno: %d\n", errno);

    uint32_t file_size; 
    if ((numbytes = recv(sockfd, &file_size, 4, 0)) == -1) 
        fprintf(stderr, "Error while receiving file_size\n errno: %d\n", errno);
    if (file_size > 0) {
        int64_t left = ntohl(file_size); char buffer[65536];
        while (left > 0) {
            uint64_t b = left;
            if (b > 65536)
                b = 65536;
            if ((numbytes = recv(sockfd, &buffer, b, 0)) == -1) 
                fprintf(stderr, "Error while receiving buffer\n errno: %d\n", errno);
            left -= numbytes;
        } 
    }

    // Stop timer print elapsed time
    struct timeval end_at;
    get_current_clock(&end_at);
    struct timeval diff_time;
    timersub(&end_at, &start_at, &diff_time);
    printf("response_time=%"PRIu64"\n", get_us(&diff_time));

    close(sockfd);
    free(key);
    free(arguments);
    pthread_exit(0);
}



void encrypt_file(uint32_t *encrypted_file, uint32_t *file, uint32_t file_size, uint32_t *key, uint32_t key_size) {
    #if OPTIM == 0 
        uint32_t file_div_key = file_size / key_size;
        uint32_t l, m, i, j, k, index_encry;
        for (l = 0; l < file_div_key; l++) {
            for (m = 0; m < file_div_key; m++) { 
                for (i = 0; i < key_size; i++) {
                    for (j = 0; j < key_size; j++) {
                        index_encry = ((l * key_size + i) * file_size) + (m * key_size + j);
                        encrypted_file[index_encry] = 0;
                        for(k = 0; k < key_size; k++) {
                            encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                        }
                    }
                }
            }
        }
    #elif OPTIM == 1
        uint32_t r, i, j, k, index_file, key_block;
        for (i = 0; i < file_size; i++) {
            key_block = (i % key_size) * key_size;
            index_file = (i-(i%key_size))*file_size;
            for(j = 0; j < file_size; j++) {
                encrypted_file[i*file_size + j] = key[key_block] * file[index_file + j];
            }
            for (k = 1; k < key_size; k++) {
                r = key[key_block + k];
                index_file = (i-(i%key_size)+k)*file_size;
                for(j = 0; j < file_size; j++) {
                    encrypted_file[i*file_size + j] += r * file[index_file+j];
                }
            }
        }
    #elif OPTIM == 2
        uint32_t file_div_key = file_size / key_size;
        uint32_t l, m, i, j, k, index_encry;
        for (l = 0; l < file_div_key; l++) {
            for (m = 0; m < file_div_key; m++) { 
                for (i = 0; i < key_size; i++) {
                    for (j = 0; j < key_size; j++) { 
                        index_encry = ((l * key_size + i) * file_size) + (m * key_size + j);
                        encrypted_file[index_encry] = 0;
                        if (key_size >= 32) {
                            for(k = 0; k < key_size; k+=32) {                    
                                encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+1] * file[((l * key_size + k+1) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+2] * file[((l * key_size + k+2) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+3] * file[((l * key_size + k+3) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+4] * file[((l * key_size + k+4) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+5] * file[((l * key_size + k+5) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+6] * file[((l * key_size + k+6) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+7] * file[((l * key_size + k+7) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+8] * file[((l * key_size + k+8) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+9] * file[((l * key_size + k+9) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+10] * file[((l * key_size + k+10) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+11] * file[((l * key_size + k+11) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+12] * file[((l * key_size + k+12) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+13] * file[((l * key_size + k+13) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+14] * file[((l * key_size + k+14) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+15] * file[((l * key_size + k+15) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+16] * file[((l * key_size + k+16) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+17] * file[((l * key_size + k+17) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+18] * file[((l * key_size + k+18) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+19] * file[((l * key_size + k+19) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+20] * file[((l * key_size + k+20) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+21] * file[((l * key_size + k+21) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+22] * file[((l * key_size + k+22) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+23] * file[((l * key_size + k+23) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+24] * file[((l * key_size + k+24) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+25] * file[((l * key_size + k+25) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+26] * file[((l * key_size + k+26) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+27] * file[((l * key_size + k+27) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+28] * file[((l * key_size + k+28) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+29] * file[((l * key_size + k+29) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+30] * file[((l * key_size + k+30) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+31] * file[((l * key_size + k+31) * file_size) + (m * key_size + j)];
                            }
                        } else if (key_size >= 8) {
                            for(k = 0; k < key_size; k+=8) {
                                encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+1] * file[((l * key_size + k+1) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+2] * file[((l * key_size + k+2) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+3] * file[((l * key_size + k+3) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+4] * file[((l * key_size + k+4) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+5] * file[((l * key_size + k+5) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+6] * file[((l * key_size + k+6) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+7] * file[((l * key_size + k+7) * file_size) + (m * key_size + j)];
                            }  
                        } else {
                            for(k = 0; k < key_size; k++) {
                                encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                            }  
                        }
                    }
                }
            }
        }
    #else
        uint32_t i, j, k, r, key_block, index_encry, index_file;
        index_encry = 0;
        uint8_t exp_key = log2(key_size);
        uint8_t exp_file = log2(file_size);
        uint8_t exp_keyblock;
        uint8_t exp_r;
        for (i = 0; i < file_size; i++) {
            key_block = (i % key_size) << exp_key;
            index_file = (i-(i%key_size)) << exp_file;
            if ((key_block != 0) && ((key_block & (key_block - 1)) == 0)) { // is power of 2, then bitshift
                exp_keyblock = log2(key_block); 
                for(j = 0; j < file_size; j+=2) {
                    encrypted_file[index_encry + j] = file[index_file + j] << exp_keyblock;
                    encrypted_file[index_encry + j+1] = file[index_file + j+1] << exp_keyblock;
                }
            } else {
                for(j = 0; j < file_size; j+=2) {
                    encrypted_file[index_encry + j] = key[key_block] * file[index_file + j];
                    encrypted_file[index_encry + j+1] = key[key_block] * file[index_file + j+1];
                }
            } 
            for (k = 1; k < key_size; k++) {
                r = key[key_block + k];
                index_file = (i-(i%key_size)+k) << exp_file;
                if ((r != 0) && ((r & (r - 1)) == 0)) { // is power of 2, then bitshift
                    exp_r = log2(r); 
                    for(j = 0; j < file_size; j+=2) {
                        encrypted_file[index_encry + j] += file[index_file+j] << exp_r;
                        encrypted_file[index_encry + j+1] += file[index_file+j+1] << exp_r;
                    }
                } else {
                    for(j = 0; j < file_size; j+=2) {
                        encrypted_file[index_encry + j] += r * file[index_file+j];
                        encrypted_file[index_encry + j+1] += r * file[index_file+j+1];        
                    }
                }
            }
            index_encry += file_size;
        }
    #endif
}
