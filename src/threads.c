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
    #elif OPTIM == 1
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
    #elif OPTIM == 2
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
    #else
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
    #endif
}
