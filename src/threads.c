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
                        if (key_size >= 2) {
                            for(k = 0; k < key_size; k+=2) {                    
                                encrypted_file[index_encry] += key[(i * key_size) + k] * file[((l * key_size + k) * file_size) + (m * key_size + j)];
                                encrypted_file[index_encry] += key[(i * key_size) + k+1] * file[((l * key_size + k+1) * file_size) + (m * key_size + j)];
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
                for(j = 0; j < file_size; j+=64) {
                    encrypted_file[index_encry + j] = file[index_file + j] << exp_keyblock;
                    encrypted_file[index_encry + j+1] = file[index_file + j+1] << exp_keyblock;
                    encrypted_file[index_encry + j+2] = file[index_file + j+2] << exp_keyblock;
                    encrypted_file[index_encry + j+3] = file[index_file + j+3] << exp_keyblock;
                    encrypted_file[index_encry + j+4] = file[index_file + j+4] << exp_keyblock;
                    encrypted_file[index_encry + j+5] = file[index_file + j+5] << exp_keyblock;
                    encrypted_file[index_encry + j+6] = file[index_file + j+6] << exp_keyblock;
                    encrypted_file[index_encry + j+7] = file[index_file + j+7] << exp_keyblock;
                    encrypted_file[index_encry + j+8] = file[index_file + j+8] << exp_keyblock;
                    encrypted_file[index_encry + j+9] = file[index_file + j+9] << exp_keyblock;
                    encrypted_file[index_encry + j+10] = file[index_file + j+10] << exp_keyblock;
                    encrypted_file[index_encry + j+11] = file[index_file + j+11] << exp_keyblock;
                    encrypted_file[index_encry + j+12] = file[index_file + j+12] << exp_keyblock;
                    encrypted_file[index_encry + j+13] = file[index_file + j+13] << exp_keyblock;
                    encrypted_file[index_encry + j+14] = file[index_file + j+14] << exp_keyblock;
                    encrypted_file[index_encry + j+15] = file[index_file + j+15] << exp_keyblock;
                    encrypted_file[index_encry + j+16] = file[index_file + j+16] << exp_keyblock;
                    encrypted_file[index_encry + j+17] = file[index_file + j+17] << exp_keyblock;
                    encrypted_file[index_encry + j+18] = file[index_file + j+18] << exp_keyblock;
                    encrypted_file[index_encry + j+19] = file[index_file + j+19] << exp_keyblock;
                    encrypted_file[index_encry + j+20] = file[index_file + j+20] << exp_keyblock;
                    encrypted_file[index_encry + j+21] = file[index_file + j+21] << exp_keyblock;
                    encrypted_file[index_encry + j+22] = file[index_file + j+22] << exp_keyblock;
                    encrypted_file[index_encry + j+23] = file[index_file + j+23] << exp_keyblock;
                    encrypted_file[index_encry + j+24] = file[index_file + j+24] << exp_keyblock;
                    encrypted_file[index_encry + j+25] = file[index_file + j+25] << exp_keyblock;
                    encrypted_file[index_encry + j+26] = file[index_file + j+26] << exp_keyblock;
                    encrypted_file[index_encry + j+27] = file[index_file + j+27] << exp_keyblock;
                    encrypted_file[index_encry + j+28] = file[index_file + j+28] << exp_keyblock;
                    encrypted_file[index_encry + j+29] = file[index_file + j+29] << exp_keyblock;
                    encrypted_file[index_encry + j+30] = file[index_file + j+30] << exp_keyblock;
                    encrypted_file[index_encry + j+31] = file[index_file + j+31] << exp_keyblock;
                    encrypted_file[index_encry + j+32] = file[index_file + j+32] << exp_keyblock;
                    encrypted_file[index_encry + j+33] = file[index_file + j+33] << exp_keyblock;
                    encrypted_file[index_encry + j+34] = file[index_file + j+34] << exp_keyblock;
                    encrypted_file[index_encry + j+35] = file[index_file + j+35] << exp_keyblock;
                    encrypted_file[index_encry + j+36] = file[index_file + j+36] << exp_keyblock;
                    encrypted_file[index_encry + j+37] = file[index_file + j+37] << exp_keyblock;
                    encrypted_file[index_encry + j+38] = file[index_file + j+38] << exp_keyblock;
                    encrypted_file[index_encry + j+39] = file[index_file + j+39] << exp_keyblock;
                    encrypted_file[index_encry + j+40] = file[index_file + j+40] << exp_keyblock;
                    encrypted_file[index_encry + j+41] = file[index_file + j+41] << exp_keyblock;
                    encrypted_file[index_encry + j+42] = file[index_file + j+42] << exp_keyblock;
                    encrypted_file[index_encry + j+43] = file[index_file + j+43] << exp_keyblock;
                    encrypted_file[index_encry + j+44] = file[index_file + j+44] << exp_keyblock;
                    encrypted_file[index_encry + j+45] = file[index_file + j+45] << exp_keyblock;
                    encrypted_file[index_encry + j+46] = file[index_file + j+46] << exp_keyblock;
                    encrypted_file[index_encry + j+47] = file[index_file + j+47] << exp_keyblock;
                    encrypted_file[index_encry + j+48] = file[index_file + j+48] << exp_keyblock;
                    encrypted_file[index_encry + j+49] = file[index_file + j+49] << exp_keyblock;
                    encrypted_file[index_encry + j+50] = file[index_file + j+50] << exp_keyblock;
                    encrypted_file[index_encry + j+51] = file[index_file + j+51] << exp_keyblock;
                    encrypted_file[index_encry + j+52] = file[index_file + j+52] << exp_keyblock;
                    encrypted_file[index_encry + j+53] = file[index_file + j+53] << exp_keyblock;
                    encrypted_file[index_encry + j+54] = file[index_file + j+54] << exp_keyblock;
                    encrypted_file[index_encry + j+55] = file[index_file + j+55] << exp_keyblock;
                    encrypted_file[index_encry + j+56] = file[index_file + j+56] << exp_keyblock;
                    encrypted_file[index_encry + j+57] = file[index_file + j+57] << exp_keyblock;
                    encrypted_file[index_encry + j+58] = file[index_file + j+58] << exp_keyblock;
                    encrypted_file[index_encry + j+59] = file[index_file + j+59] << exp_keyblock;
                    encrypted_file[index_encry + j+60] = file[index_file + j+60] << exp_keyblock;
                    encrypted_file[index_encry + j+61] = file[index_file + j+61] << exp_keyblock;
                    encrypted_file[index_encry + j+62] = file[index_file + j+62] << exp_keyblock;
                    encrypted_file[index_encry + j+63] = file[index_file + j+63] << exp_keyblock;
                }
            } else {
                for(j = 0; j < file_size; j+=64) {
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
                    encrypted_file[index_encry + j+32] = key[key_block] * file[index_file + j+32];
                    encrypted_file[index_encry + j+33] = key[key_block] * file[index_file + j+33];
                    encrypted_file[index_encry + j+34] = key[key_block] * file[index_file + j+34];
                    encrypted_file[index_encry + j+35] = key[key_block] * file[index_file + j+35];
                    encrypted_file[index_encry + j+36] = key[key_block] * file[index_file + j+36];
                    encrypted_file[index_encry + j+37] = key[key_block] * file[index_file + j+37];
                    encrypted_file[index_encry + j+38] = key[key_block] * file[index_file + j+38];
                    encrypted_file[index_encry + j+39] = key[key_block] * file[index_file + j+39];
                    encrypted_file[index_encry + j+40] = key[key_block] * file[index_file + j+40];
                    encrypted_file[index_encry + j+41] = key[key_block] * file[index_file + j+41];
                    encrypted_file[index_encry + j+42] = key[key_block] * file[index_file + j+42];
                    encrypted_file[index_encry + j+43] = key[key_block] * file[index_file + j+43];
                    encrypted_file[index_encry + j+44] = key[key_block] * file[index_file + j+44];
                    encrypted_file[index_encry + j+45] = key[key_block] * file[index_file + j+45];
                    encrypted_file[index_encry + j+46] = key[key_block] * file[index_file + j+46];
                    encrypted_file[index_encry + j+47] = key[key_block] * file[index_file + j+47];
                    encrypted_file[index_encry + j+48] = key[key_block] * file[index_file + j+48];
                    encrypted_file[index_encry + j+49] = key[key_block] * file[index_file + j+49];
                    encrypted_file[index_encry + j+50] = key[key_block] * file[index_file + j+50];
                    encrypted_file[index_encry + j+51] = key[key_block] * file[index_file + j+51];
                    encrypted_file[index_encry + j+52] = key[key_block] * file[index_file + j+52];
                    encrypted_file[index_encry + j+53] = key[key_block] * file[index_file + j+53];
                    encrypted_file[index_encry + j+54] = key[key_block] * file[index_file + j+54];
                    encrypted_file[index_encry + j+55] = key[key_block] * file[index_file + j+55];
                    encrypted_file[index_encry + j+56] = key[key_block] * file[index_file + j+56];
                    encrypted_file[index_encry + j+57] = key[key_block] * file[index_file + j+57];
                    encrypted_file[index_encry + j+58] = key[key_block] * file[index_file + j+58];
                    encrypted_file[index_encry + j+59] = key[key_block] * file[index_file + j+59];
                    encrypted_file[index_encry + j+60] = key[key_block] * file[index_file + j+60];
                    encrypted_file[index_encry + j+61] = key[key_block] * file[index_file + j+61];
                    encrypted_file[index_encry + j+62] = key[key_block] * file[index_file + j+62];
                    encrypted_file[index_encry + j+63] = key[key_block] * file[index_file + j+63];
                }
            } 
            for (k = 1; k < key_size; k++) {
                r = key[key_block + k];
                index_file = (i-(i%key_size)+k) << exp_file;
                if ((r != 0) && ((r & (r - 1)) == 0)) { // is power of 2, then bitshift
                    exp_r = log2(r); 
                    for(j = 0; j < file_size; j+=64) {
                        encrypted_file[index_encry + j] += file[index_file+j] << exp_r;
                        encrypted_file[index_encry + j+1] += file[index_file+j+1] << exp_r;
                        encrypted_file[index_encry + j+2] += file[index_file+j+2] << exp_r;
                        encrypted_file[index_encry + j+3] += file[index_file+j+3] << exp_r;
                        encrypted_file[index_encry + j+4] += file[index_file+j+4] << exp_r;
                        encrypted_file[index_encry + j+5] += file[index_file+j+5] << exp_r;
                        encrypted_file[index_encry + j+6] += file[index_file+j+6] << exp_r;
                        encrypted_file[index_encry + j+7] += file[index_file+j+7] << exp_r;
                        encrypted_file[index_encry + j+8] += file[index_file+j+8] << exp_r;
                        encrypted_file[index_encry + j+9] += file[index_file+j+9] << exp_r;
                        encrypted_file[index_encry + j+10] += file[index_file+j+10] << exp_r;
                        encrypted_file[index_encry + j+11] += file[index_file+j+11] << exp_r;
                        encrypted_file[index_encry + j+12] += file[index_file+j+12] << exp_r;
                        encrypted_file[index_encry + j+13] += file[index_file+j+13] << exp_r;
                        encrypted_file[index_encry + j+14] += file[index_file+j+14] << exp_r;
                        encrypted_file[index_encry + j+15] += file[index_file+j+15] << exp_r;
                        encrypted_file[index_encry + j+16] += file[index_file+j+16] << exp_r;
                        encrypted_file[index_encry + j+17] += file[index_file+j+17] << exp_r;
                        encrypted_file[index_encry + j+18] += file[index_file+j+18] << exp_r;
                        encrypted_file[index_encry + j+19] += file[index_file+j+19] << exp_r;
                        encrypted_file[index_encry + j+20] += file[index_file+j+20] << exp_r;
                        encrypted_file[index_encry + j+21] += file[index_file+j+21] << exp_r;
                        encrypted_file[index_encry + j+22] += file[index_file+j+22] << exp_r;
                        encrypted_file[index_encry + j+23] += file[index_file+j+23] << exp_r;
                        encrypted_file[index_encry + j+24] += file[index_file+j+24] << exp_r;
                        encrypted_file[index_encry + j+25] += file[index_file+j+25] << exp_r;
                        encrypted_file[index_encry + j+26] += file[index_file+j+26] << exp_r;
                        encrypted_file[index_encry + j+27] += file[index_file+j+27] << exp_r;
                        encrypted_file[index_encry + j+28] += file[index_file+j+28] << exp_r;
                        encrypted_file[index_encry + j+29] += file[index_file+j+29] << exp_r;
                        encrypted_file[index_encry + j+30] += file[index_file+j+30] << exp_r;
                        encrypted_file[index_encry + j+31] += file[index_file+j+31] << exp_r;
                        encrypted_file[index_encry + j+32] += file[index_file+j+32] << exp_r;
                        encrypted_file[index_encry + j+33] += file[index_file+j+33] << exp_r;
                        encrypted_file[index_encry + j+34] += file[index_file+j+34] << exp_r;
                        encrypted_file[index_encry + j+35] += file[index_file+j+35] << exp_r;
                        encrypted_file[index_encry + j+36] += file[index_file+j+36] << exp_r;
                        encrypted_file[index_encry + j+37] += file[index_file+j+37] << exp_r;
                        encrypted_file[index_encry + j+38] += file[index_file+j+38] << exp_r;
                        encrypted_file[index_encry + j+39] += file[index_file+j+39] << exp_r;
                        encrypted_file[index_encry + j+40] += file[index_file+j+40] << exp_r;
                        encrypted_file[index_encry + j+41] += file[index_file+j+41] << exp_r;
                        encrypted_file[index_encry + j+42] += file[index_file+j+42] << exp_r;
                        encrypted_file[index_encry + j+43] += file[index_file+j+43] << exp_r;
                        encrypted_file[index_encry + j+44] += file[index_file+j+44] << exp_r;
                        encrypted_file[index_encry + j+45] += file[index_file+j+45] << exp_r;
                        encrypted_file[index_encry + j+46] += file[index_file+j+46] << exp_r;
                        encrypted_file[index_encry + j+47] += file[index_file+j+47] << exp_r;
                        encrypted_file[index_encry + j+48] += file[index_file+j+48] << exp_r;
                        encrypted_file[index_encry + j+49] += file[index_file+j+49] << exp_r;
                        encrypted_file[index_encry + j+50] += file[index_file+j+50] << exp_r;
                        encrypted_file[index_encry + j+51] += file[index_file+j+51] << exp_r;
                        encrypted_file[index_encry + j+52] += file[index_file+j+52] << exp_r;
                        encrypted_file[index_encry + j+53] += file[index_file+j+53] << exp_r;
                        encrypted_file[index_encry + j+54] += file[index_file+j+54] << exp_r;
                        encrypted_file[index_encry + j+55] += file[index_file+j+55] << exp_r;
                        encrypted_file[index_encry + j+56] += file[index_file+j+56] << exp_r;
                        encrypted_file[index_encry + j+57] += file[index_file+j+57] << exp_r;
                        encrypted_file[index_encry + j+58] += file[index_file+j+58] << exp_r;
                        encrypted_file[index_encry + j+59] += file[index_file+j+59] << exp_r;
                        encrypted_file[index_encry + j+60] += file[index_file+j+60] << exp_r;
                        encrypted_file[index_encry + j+61] += file[index_file+j+61] << exp_r;
                        encrypted_file[index_encry + j+62] += file[index_file+j+62] << exp_r;
                        encrypted_file[index_encry + j+63] += file[index_file+j+63] << exp_r;
                    }
                } else {
                    for(j = 0; j < file_size; j+=64) {
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
                        encrypted_file[index_encry + j+32] += r * file[index_file+j+32];
                        encrypted_file[index_encry + j+33] += r * file[index_file+j+33];
                        encrypted_file[index_encry + j+34] += r * file[index_file+j+34];
                        encrypted_file[index_encry + j+35] += r * file[index_file+j+35];
                        encrypted_file[index_encry + j+36] += r * file[index_file+j+36];
                        encrypted_file[index_encry + j+37] += r * file[index_file+j+37];
                        encrypted_file[index_encry + j+38] += r * file[index_file+j+38];
                        encrypted_file[index_encry + j+39] += r * file[index_file+j+39];
                        encrypted_file[index_encry + j+40] += r * file[index_file+j+40];
                        encrypted_file[index_encry + j+41] += r * file[index_file+j+41];
                        encrypted_file[index_encry + j+42] += r * file[index_file+j+42];
                        encrypted_file[index_encry + j+43] += r * file[index_file+j+43];
                        encrypted_file[index_encry + j+44] += r * file[index_file+j+44];
                        encrypted_file[index_encry + j+45] += r * file[index_file+j+45];
                        encrypted_file[index_encry + j+46] += r * file[index_file+j+46];
                        encrypted_file[index_encry + j+47] += r * file[index_file+j+47];
                        encrypted_file[index_encry + j+48] += r * file[index_file+j+48];
                        encrypted_file[index_encry + j+49] += r * file[index_file+j+49];
                        encrypted_file[index_encry + j+50] += r * file[index_file+j+50];
                        encrypted_file[index_encry + j+51] += r * file[index_file+j+51];
                        encrypted_file[index_encry + j+52] += r * file[index_file+j+52];
                        encrypted_file[index_encry + j+53] += r * file[index_file+j+53];
                        encrypted_file[index_encry + j+54] += r * file[index_file+j+54];
                        encrypted_file[index_encry + j+55] += r * file[index_file+j+55];
                        encrypted_file[index_encry + j+56] += r * file[index_file+j+56];
                        encrypted_file[index_encry + j+57] += r * file[index_file+j+57];
                        encrypted_file[index_encry + j+58] += r * file[index_file+j+58];
                        encrypted_file[index_encry + j+59] += r * file[index_file+j+59];
                        encrypted_file[index_encry + j+60] += r * file[index_file+j+60];
                        encrypted_file[index_encry + j+61] += r * file[index_file+j+61];
                        encrypted_file[index_encry + j+62] += r * file[index_file+j+62];
                        encrypted_file[index_encry + j+63] += r * file[index_file+j+63];
                    }
                }
            }
            index_encry += file_size;
        }
    #endif
}
