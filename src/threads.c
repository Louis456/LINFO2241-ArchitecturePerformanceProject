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
        uint32_t index_encry;
        uint32_t file_div_key = file_size / key_size;
        for (uint32_t l = 0; l < file_div_key; l++) {
            for (uint32_t m = 0; m < file_div_key; m++) { 
                for (uint32_t i = 0; i < key_size; i++) {
                    for (uint32_t j = 0; j < key_size; j++) {
                        index_encry = ((l * key_size + i) * file_size) + (m * key_size + j);
                        encrypted_file[index_encry] = 0;
                        for(uint32_t k = 0; k < key_size; k++) {
                            encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                        }
                    }
                }
            }
        }
    #elif OPTIM == 1
        uint32_t key_block;
        uint32_t index_file;
        uint32_t r;
        for (uint32_t i = 0; i < file_size; i++) {
            key_block = (i % key_size) * key_size;
            index_file = (i-(i%key_size))*file_size;
            for(uint32_t j = 0; j < file_size; j++) {
                encrypted_file[i*file_size + j] = key[key_block] * file[index_file + j];
            }
            for (uint32_t k = 1; k < key_size; k++) {
                r = key[key_block + k];
                index_file = (i-(i%key_size)+k)*file_size;
                for(uint32_t j = 0; j < file_size; j++) {
                    encrypted_file[i*file_size + j] += r * file[index_file+j];
                }
            }
        }
    #elif OPTIM == 2
        uint32_t file_div_key = file_size / key_size;
        uint32_t index_encry;
        for (uint32_t l = 0; l < file_div_key; l++) {
            for (uint32_t m = 0; m < file_div_key; m++) { 
                for (uint32_t i = 0; i < key_size; i++) {
                    for (uint32_t j = 0; j < key_size; j++) { 
                        index_encry = ((l * key_size + i) * file_size) + (m * key_size + j);
                        encrypted_file[index_encry] = 0;
                        if (key_size >= 8) {
                            for(uint32_t k = 0; k < key_size; k+=8) {                    
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
                            for(uint32_t k = 0; k < key_size; k++) {
                                encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                            }  
                        }
                    }
                }
            }
        }
    #else
        uint32_t r;
        uint32_t key_block;
        uint32_t index_encry;
        uint32_t index_file;
        for (uint32_t i = 0; i < file_size; i++) {
            key_block = (i % key_size) * key_size;
            index_encry = i*file_size;
            index_file = (i-(i%key_size))*file_size;
            for(uint32_t j = 0; j < file_size; j+=32) {
                encrypted_file[index_encry + j] = key[key_block] * file[index_file + j];
                encrypted_file[index_encry + j+1] = key[key_block] * file[index_file + j+1];  
                encrypted_file[index_encry + j+2] = key[key_block] * file[index_file + j+2];
                encrypted_file[index_encry + j+3] = key[key_block] * file[index_file + j+3];
                encrypted_file[index_encry + j+4] = key[key_block] * file[index_file + j+4];
                encrypted_file[index_encry + j+5] = key[key_block] * file[index_file + j+5];  
                encrypted_file[index_encry + j+6] = key[key_block] * file[index_file + j+6];
                encrypted_file[index_encry + j+7] = key[key_block] * file[index_file + j+7]; 
                encrypted_file[index_encry + j+8] = key[key_block] * file[index_file + j+8];
                encrypted_file[index_encry + j+9] = key[key_block] * file[index_file + j+9];  
                encrypted_file[index_encry + j+10] = key[key_block] * file[index_file + j+10];
                encrypted_file[index_encry + j+11] = key[key_block] * file[index_file + j+11];
                encrypted_file[index_encry + j+12] = key[key_block] * file[index_file + j+12];
                encrypted_file[index_encry + j+13] = key[key_block] * file[index_file + j+13];  
                encrypted_file[index_encry + j+14] = key[key_block] * file[index_file + j+14];
                encrypted_file[index_encry + j+15] = key[key_block] * file[index_file + j+15];
                encrypted_file[index_encry + j+16] = key[key_block] * file[index_file + j+16];
                encrypted_file[index_encry + j+17] = key[key_block] * file[index_file + j+17];  
                encrypted_file[index_encry + j+18] = key[key_block] * file[index_file + j+18];
                encrypted_file[index_encry + j+19] = key[key_block] * file[index_file + j+19];
                encrypted_file[index_encry + j+20] = key[key_block] * file[index_file + j+20];
                encrypted_file[index_encry + j+21] = key[key_block] * file[index_file + j+21];  
                encrypted_file[index_encry + j+22] = key[key_block] * file[index_file + j+22];
                encrypted_file[index_encry + j+23] = key[key_block] * file[index_file + j+23]; 
                encrypted_file[index_encry + j+24] = key[key_block] * file[index_file + j+24];
                encrypted_file[index_encry + j+25] = key[key_block] * file[index_file + j+25];  
                encrypted_file[index_encry + j+26] = key[key_block] * file[index_file + j+26];
                encrypted_file[index_encry + j+27] = key[key_block] * file[index_file + j+27];
                encrypted_file[index_encry + j+28] = key[key_block] * file[index_file + j+28];
                encrypted_file[index_encry + j+29] = key[key_block] * file[index_file + j+29];  
                encrypted_file[index_encry + j+30] = key[key_block] * file[index_file + j+30];
                encrypted_file[index_encry + j+31] = key[key_block] * file[index_file + j+31];                  
            }
            for (uint32_t k = 1; k < key_size; k++) {
                r = key[key_block + k];
                index_file = (i-(i%key_size)+k)*file_size;
                for(uint32_t j = 0; j < file_size; j+=32) {
                    encrypted_file[index_encry + j] += r * file[index_file+j];
                    encrypted_file[index_encry + j+1] += r * file[index_file+j+1];
                    encrypted_file[index_encry + j+2] += r * file[index_file+j+2];
                    encrypted_file[index_encry + j+3] += r * file[index_file+j+3];
                    encrypted_file[index_encry + j+4] += r * file[index_file+j+4];
                    encrypted_file[index_encry + j+5] += r * file[index_file+j+5];
                    encrypted_file[index_encry + j+6] += r * file[index_file+j+6];
                    encrypted_file[index_encry + j+7] += r * file[index_file+j+7];
                    encrypted_file[index_encry + j+8] += r * file[index_file+j+8];
                    encrypted_file[index_encry + j+9] += r * file[index_file+j+9];
                    encrypted_file[index_encry + j+10] += r * file[index_file+j+10];
                    encrypted_file[index_encry + j+11] += r * file[index_file+j+11];
                    encrypted_file[index_encry + j+12] += r * file[index_file+j+12];
                    encrypted_file[index_encry + j+13] += r * file[index_file+j+13];
                    encrypted_file[index_encry + j+14] += r * file[index_file+j+14];
                    encrypted_file[index_encry + j+15] += r * file[index_file+j+15];
                    encrypted_file[index_encry + j+16] += r * file[index_file+j+16];
                    encrypted_file[index_encry + j+17] += r * file[index_file+j+17];
                    encrypted_file[index_encry + j+18] += r * file[index_file+j+18];
                    encrypted_file[index_encry + j+19] += r * file[index_file+j+19];
                    encrypted_file[index_encry + j+20] += r * file[index_file+j+20];
                    encrypted_file[index_encry + j+21] += r * file[index_file+j+21];
                    encrypted_file[index_encry + j+22] += r * file[index_file+j+22];
                    encrypted_file[index_encry + j+23] += r * file[index_file+j+23];
                    encrypted_file[index_encry + j+24] += r * file[index_file+j+24];
                    encrypted_file[index_encry + j+25] += r * file[index_file+j+25];
                    encrypted_file[index_encry + j+26] += r * file[index_file+j+26];
                    encrypted_file[index_encry + j+27] += r * file[index_file+j+27];
                    encrypted_file[index_encry + j+28] += r * file[index_file+j+28];
                    encrypted_file[index_encry + j+29] += r * file[index_file+j+29];
                    encrypted_file[index_encry + j+30] += r * file[index_file+j+30];
                    encrypted_file[index_encry + j+31] += r * file[index_file+j+31];
                }
            }
        }
    #endif
}
