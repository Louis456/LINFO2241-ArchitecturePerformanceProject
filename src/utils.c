#include "../headers/utils.h"

void get_current_clock(struct timeval *timestamp) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		fprintf(stderr, "Cannot get internal clock");
	}
    timestamp->tv_sec = ts.tv_sec;
	timestamp->tv_usec = ts.tv_nsec/1000;
}

uint64_t get_ms(struct timeval *timestamp) {
    return timestamp->tv_sec * 1000 + timestamp->tv_usec / 1000;
}

uint64_t get_us(struct timeval *timestamp) {
    return timestamp->tv_sec * 1000000 + timestamp->tv_usec;
}

uint64_t ran_expo(double lambda){
	double u;
	u = rand() / (RAND_MAX + 1.0);
	return -log(1- u) * 1000000000 / lambda;
}