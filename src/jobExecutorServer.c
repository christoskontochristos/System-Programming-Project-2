
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>


// #include "shared_data.h"
#include "buffer.h"
#include "Info.h"
#include "utility.h"
#include "threads.h"





//function to initalize the server information
Server_Info * Init_Server(int PORT, int buffer_size, int thread_pool_size){

    Server_Info * info = malloc(sizeof(Server_Info));
    if(info == NULL){
        perror("malloc");
        return NULL;
    }

    info->job_counter = 0;
    info->loop_flag = true;
    info->PORT_NUM = PORT;
    info->thread_pool_size = thread_pool_size;
    info->SERVER_NAME = "\0";
    info->server_socket = -1;
    info->buffer_size = buffer_size;

    info->worker_threads = malloc(sizeof(pthread_t) * info->thread_pool_size);
    if(info->worker_threads == NULL){
        perror("malloc");
        return NULL;
    }

    for(int i = 0 ; i < info->thread_pool_size ; i++){
        info->worker_threads[i] = 0;
    }

    return info;

}



//main function
int main(int argc, char * argv[]){

    if(argc != 4){
        printf("Correct usage : ./jobExecuorServer [port number (int)] [buffer size (int)] [thread pool size (int)]\n");
        return 1;
    }

    if(!IsInt(argv[1]) || !IsInt(argv[2]) || !IsInt(argv[3])){
        printf("The argumets must all be integers!\n");
        return 1;           
    }

    if(argv[1] < 0 || argv[2] < 0 || argv[3] < 0){
        printf("The arguments must be positive integeters!\n");
        return 1;
    }


    Shared_Data * sh_data;

    //Initalize the server
    Server_Info * info = Init_Server(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    if(info == NULL){
        printf("An error occured while initializing the server!\n");
        return 1;
    }

    //Run the Server (the main thread)

    //create and run the main thread
    pthread_t tid = Create_Thread(main_thread, info);

    //join the main thread
    Join_Thread(tid);


    return 0;

}

