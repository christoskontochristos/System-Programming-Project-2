#ifndef THREADS_H
#define THREADS_H

// #include "shared_data.h"
#include "Info.h"
#include "buffer.h"

//struct that holds the controller arguments that are passed in the controller thread
typedef struct{

    Server_Info * info;
    char * command;
    char * args;
    int clientSocket;

}Controller_Args;

//thread prototypes
void * main_thread(void * args);
void * worker_thread();
void * controller_thread(void * args);


#endif // THREADS_H