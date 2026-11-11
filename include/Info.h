#ifndef INFO_H
#define INFO_H

#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>


//jobCommander Info
typedef struct{

    int PORT_NUM;
    char * SERVER_HOST_ADDR;
    char * command;
    char * command_args;

}Commander_Info;

//jobExecutorServer Info 
typedef struct{

    int PORT_NUM;
    char * SERVER_NAME;
    int job_counter;
    bool loop_flag;
    int thread_pool_size;
    pthread_t * worker_threads;
    int server_socket;
    int buffer_size;
    struct sockaddr_storage serverStorage;

}Server_Info;

#endif // INFO_H