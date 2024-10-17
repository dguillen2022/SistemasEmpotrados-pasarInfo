#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>

#define PRIORITY 99
#define _GNU_SOURCE
#define SLEEP_TIME 0.5

struct data {
    int cpu
    double *latency
};

void *thread_function(void *num_thread) {

    struct timespec init_time, final_time;
    struct data thread_info;
    double init_seconds, final_seconds, cost;
    int id_clock=CLOCK_MONOTONIC

    int policy=SCHED_FIFO;
    struct sched_param param;
    param.sched_priority = PRIORITY;

    //latencies = 

    int cpu_id=thread_info.cpu;

    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    int s1=pthread_setschedparam(pthread_self(), policy, &param);
    int s2=pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

    if (s1 != 0)
        errx(EXIT_FAILURE, "pthread_setschedparam failed: %d", s1);

    if (s2 != 0)
        errx(EXIT_FAILURE, "pthread_setaffinity_np failed: %d", s2);

    if (clock_gettime(id_clock, &init_time) != 0) {
        perror("Error al obtener tiempo final");
        exit(-1);
    }
    init_seconds=init_time.tv_sec+init_time.tv_nsec/1e9;

    usleep(SLEEP_TIME * 1e6);

    if (clock_gettime(id_clock, &final_time) != 0) {
            perror("Error al obtener tiempo final");
            exit(-1);
    }
    final_seconds=final_time.tv_sec+final_time.tv_nsec/1e9;

    thread_info.latency=final_seconds - init_seconds - SLEEP_TIME;

   
}

int main(int argc, char *argv[]) {

    static int32_t latency_target_value = 0;
    int latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    write(latency_target_fd, &latency_target_value, 4);

    int num_cores = (int) sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t array_threads[num_cores];
    int num_thread_array[num_cores];

    struct data *thread_info[num_cores];

    for (int i = 0; i < num_cores; i++){
        thread_info[i]->cpu = i;
        pthread_create(&array_threads[i], NULL, thread_function,
             NULL);
    }

    for (int i = 0; i < N; i++){
        pthread_join(array_threads[i], NULL);
    }
    
    pthread_exit(NULL);
    
    return 0;
}
