#define _GNU_SOURCE
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
#define SLEEP_TIME 0.5e9
#define DURATION_NS 60e9

struct data {
    int cpu;
    double max_latency;
    double avg_latency;
    double *latencies;
    int iteration;
};

void *thread_function(void *thread_info_ptr) {
    struct data *thread_info = (struct data *)thread_info_ptr;
    struct timespec init_time, final_time, init_thread_time;

    long init_ns, final_ns=0, init_thread_ns, latency, total_latency=0; 
    long max_latency=0, avg_latency;
    int iterations=0, max_iterations;
    int id_clock=CLOCK_MONOTONIC;

    int policy=SCHED_FIFO;
    struct sched_param param;
    param.sched_priority = PRIORITY;

    int cpu_id=thread_info->cpu;
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    int s1=pthread_setschedparam(pthread_self(), policy, &param);
    int s2=pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

    if (s1 != 0)
        errx(EXIT_FAILURE, "pthread_setschedparam failed: %d", s1);

    if (s2 != 0)
        errx(EXIT_FAILURE, "pthread_setaffinity_np failed: %d", s2);

    max_iterations = DURATION_NS/SLEEP_TIME;
    thread_info->latencies=(double *) malloc(sizeof(double)*max_iterations);

    if (clock_gettime(id_clock, &init_thread_time) != 0) {
        perror("Error al obtener tiempo final");
        exit(-1);
    }
    init_thread_ns=init_thread_time.tv_sec*1e9+init_thread_time.tv_nsec;

    while (final_ns - init_thread_ns < DURATION_NS) {
        if (clock_gettime(id_clock, &init_time) != 0) {
            perror("Error al obtener tiempo final");
            exit(-1);
        }
        init_ns = init_time.tv_sec*1e9 + init_time.tv_nsec;

        usleep(SLEEP_TIME/1e3);

        if (clock_gettime(id_clock, &final_time) != 0) {
                perror("Error al obtener tiempo final");
                exit(-1);
        }
        final_ns=final_time.tv_sec*1e9+final_time.tv_nsec;

        latency=final_ns-init_ns-(SLEEP_TIME);
        total_latency+=latency;
        thread_info->latencies[iterations]=latency;

        if (latency>max_latency){
            max_latency=latency;
        }

        iterations++;
        if (iterations >= max_iterations) {
            break;
        }
    }
    thread_info->iteration=iterations;
    thread_info->avg_latency=total_latency/iterations;
    thread_info->max_latency=max_latency;

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    static int32_t latency_target_value = 0;
    int latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    write(latency_target_fd, &latency_target_value, 4);

    int num_cores = (int) sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t array_threads[num_cores];
    int num_thread_array[num_cores];

    long double total_avg_latency=0, total_max_latency=0, avg_total_avg_latency;
    int avg_latency_ns, max_latency_ns, iterations;


    struct data *thread_info[num_cores];

    for (int i = 0; i < num_cores; i++){
        thread_info[i]=(struct data *) malloc(sizeof(struct data));
        thread_info[i]->cpu = i;
        pthread_create(&array_threads[i], NULL, thread_function,
             (void *)thread_info[i]);
    }

    for (int i = 0; i < num_cores; i++){
        pthread_join(array_threads[i], NULL);

        avg_latency_ns=(int)(thread_info[i]->avg_latency);
        max_latency_ns=(int)(thread_info[i]->max_latency);
        iterations = thread_info[i]->iteration;

        printf("[%i]    latencia media = %i ns. | max = %i ns\n", 
             i, avg_latency_ns, max_latency_ns);

        total_avg_latency+=avg_latency_ns;

        if (max_latency_ns>total_max_latency) {
            total_max_latency=max_latency_ns;
        }
    }

    avg_total_avg_latency=total_avg_latency/num_cores;
    printf("\nTotal latencia media = %i ns. | max = %i ns\n",
     (int)(avg_total_avg_latency), (int)(total_max_latency));


    FILE *csv_file = fopen("cyclictestURJC.csv", "w");
    if (csv_file == NULL) {
        perror("Error al abrir el archivo CSV");
        exit(EXIT_FAILURE);
    }
    fprintf(csv_file, "CPU,NUMERO_ITERACION,LATENCIA\n");

    for (int i=0;i<num_cores;i++) {
        for(int j=0;j<iterations;j++) {
            fprintf(csv_file, "%i,%i,%i\n", i, j, (int)(thread_info[i]->latencies[j]));
        }
    }
    fclose(csv_file);

    for (int i = 0; i < num_cores; i++) {
        free(thread_info[i]->latencies);
        free(thread_info[i]);
    }

    pthread_exit(NULL);

    return 0;
}
