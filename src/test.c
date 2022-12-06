#include "../headers/threads.h"


int FILE_SIZE = 16;
int KEY_SIZE = 8;
int ALIGNMENT = 32;

void print_matrix(float* matrix, int size){
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            printf("%f\t", matrix[(i * size) + j]);
        }
        printf("\n");
    }
}

int compare_result(float* matrix1, float* matrix2, int size){
    bool same = true;
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            same = same && (matrix1[(i*size) + j] == matrix2[(i*size) + j]);
        }
    }
    return same;
}

void matrix_no_optimization(float *encrypted_file, float *file, uint32_t file_size, float *key, uint32_t key_size) {
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
}

void matrix_mult_float(float *encrypted_file, float *file, uint32_t file_size, float *key, uint32_t key_size) {
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
}

void matrix_mult_avx(float *encrypted_file, float *file, uint32_t file_size, float *key, uint32_t key_size) {
    uint32_t i, j, k, index_file, key_block;
    float r;
    for (i = 0; i < file_size; i++) {
        key_block = (i % key_size) * key_size;
        index_file = (i-(i%key_size))*file_size;
        for(j = 0; j < file_size; j+=8) {
            __m256 avx_key = _mm256_set_ps(key[key_block],key[key_block],key[key_block],key[key_block],key[key_block],key[key_block],key[key_block],key[key_block]);
            __m256 avx_file = _mm256_load_ps(file+index_file+j);
            __m256 key_times_file = _mm256_mul_ps(avx_key, avx_file);
            _mm256_store_ps(encrypted_file+(i*file_size)+j, key_times_file);
        }
        for (k = 1; k < key_size; k++) {
            r = key[key_block + k];
            index_file = (i-(i%key_size)+k)*file_size;
            for(j = 0; j < file_size; j+=8) {
                __m256 avx_r = _mm256_set_ps(r,r,r,r,r,r,r,r);
                __m256 avx_file = _mm256_load_ps(file+index_file+j);
                __m256 avx_enc_file = _mm256_load_ps(file+(i*file_size)+j);
                __m256 r_times_file = _mm256_mul_ps(avx_r, avx_file);
                __m256 enc_file_add = _mm256_add_ps(r_times_file, avx_enc_file);
                _mm256_store_ps(encrypted_file+(i*file_size)+j, enc_file_add);
            }
        }
    }
}

int main(){
    
    float* files[1];
    float* file = (float*) aligned_alloc(FILE_SIZE, FILE_SIZE * FILE_SIZE * sizeof(float));
    float current = 1.0;
    for(int i = 0; i < FILE_SIZE * FILE_SIZE; i++){
        file[i] = current;
        current++;
    }
    files[0] = file;

    float* key = (float*) aligned_alloc(KEY_SIZE, KEY_SIZE * KEY_SIZE * sizeof(float));
    current = 1;
    for(int i = 0; i < KEY_SIZE * KEY_SIZE; i++){
        key[i] = current;
        current++;
    }

    float* encrypted_file_mult = (float*) aligned_alloc(ALIGNMENT, FILE_SIZE * FILE_SIZE * sizeof(float));
    matrix_no_optimization(encrypted_file_mult ,file, FILE_SIZE, key, KEY_SIZE);

    float* encrypted_file_float = (float*) aligned_alloc(ALIGNMENT, FILE_SIZE * FILE_SIZE * sizeof(float));
    matrix_mult_float(encrypted_file_float ,file, FILE_SIZE, key, KEY_SIZE);

    float* encrypted_file_avx = (float*) aligned_alloc(ALIGNMENT, FILE_SIZE * FILE_SIZE * sizeof(float));
    matrix_mult_avx(encrypted_file_avx ,file, FILE_SIZE, key, KEY_SIZE);

    printf("encrypted file mult\n");
    print_matrix(encrypted_file_mult, 16);
    printf("encrypted file avx\n");
    print_matrix(encrypted_file_avx, 16);

    if(compare_result(encrypted_file_mult, encrypted_file_float, FILE_SIZE)){
        printf("OK normal mult and float\n");
    } else{
        printf("NOK normal mult and float\n");
    }

    if(compare_result(encrypted_file_mult, encrypted_file_avx, FILE_SIZE)){
        printf("OK normal mult and avx\n");
    } else{
        printf("NOK normal mult and avx\n");
    }

    free(encrypted_file_mult);
    free(encrypted_file_float);
    free(encrypted_file_avx);
}