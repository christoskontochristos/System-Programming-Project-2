#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "threads.h"
#include "Info.h"
#include "utility.h"
#include "commands.h"

//Shared_Data object that holds all the shared data of the threads
Shared_Data * shared_data;

//the backlog value
#define BACKLOG_VALUE 128

//function that monitors if the worker threads should stop running or not
volatile bool stop_worker_threads = false;

//function that initializes the mutexes and condition variables
void Init_Mutexes_And_Cond_Vars(){

    pthread_mutex_init(&(shared_data->buffer_mutex), 0);

    pthread_cond_init(&(shared_data->nonempty), 0);
    pthread_cond_init(&(shared_data->nonfull), 0);

    return;
}

//function that destroys the mutexes and condition variables
void Destroy_Mutexes_And_Cond_Vars(){

    //pthread_mutex_destroy(&(shared_data->max_con_mutex));
    pthread_mutex_destroy(&(shared_data->buffer_mutex));

    pthread_cond_destroy(&(shared_data->nonempty));
    pthread_cond_destroy(&(shared_data->nonfull));

    return;
}


//function that initalizes the shared resources
bool Init_Shared_Data(int buffer_size){

    shared_data = malloc(sizeof(Shared_Data));
    if(shared_data == NULL){
        perror("malloc");
        return false;
    }

    shared_data->buffer = Create_Buffer(buffer_size);
    if(shared_data->buffer == NULL){
        return false;
    }

    shared_data->concurrency_level = 0;
    shared_data->max_concurrency = DEFAULT_CONCURRECNY;
    shared_data->job_counter = 0;

    return true;
}


//function that initializes the server socket 
void Init_Server_Socket(Server_Info * info){

    //initialize variables
    int serverSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(info->PORT_NUM);

    //bind the socket to the address and port number.
    if(bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        perror("bind");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    //listen on the socket, with 128 max connection requests queued
    if(listen(serverSocket, BACKLOG_VALUE) == 0){
        // printf("Listening\n");
    } else {
        perror("listen\n");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    info->server_socket = serverSocket;
    info->serverStorage = serverStorage;

    //printf("%d\n", info->server_socket);

    return;
}


//accept the connections from the jobCommanders
int Accept_Connections(Server_Info * info){

    socklen_t addr_size = sizeof(info->server_socket);

    // Extract the first connection in the queue
    int newSocket = accept(info->server_socket, (struct sockaddr*)&(info->serverStorage), &addr_size);
    if (newSocket < 0) {
        perror("accept");
        return -1;
    }

    return newSocket;

}

//function to read the data that a jobCommander has sent to the server through the socket 
char * Read_Data(int new_socket){

    //char *buffer = malloc(sizeof(char) * SOCKET_BUFFER_LENGTH);
    char *buffer = malloc(SOCKET_BUFFER_LENGTH);
    if(buffer == NULL){
        perror("malloc");
        return NULL;
    }

    ssize_t bytes_read = recv(new_socket, buffer, SOCKET_BUFFER_LENGTH - 1, 0);
    if(bytes_read == -1){
        perror("recv");
        free(buffer);
        return NULL;
    }

    //ensure the buffer is null-terminated
    buffer[bytes_read] = '\0';

    //printf("buffer read :%s\n", buffer);

    return buffer;
}

//function that creates and runs a controller thread 
int Spawn_Controller_Thread(char *commander_input, int socket_fd, Server_Info *info) {

    //init the controller arguments
    Controller_Args *args = malloc(sizeof(Controller_Args));
    if (args == NULL) {
        perror("malloc");
        return -1;
    }

    // printf("commander input : %s\n", commander_input);

    args->info = info;
    args->clientSocket = socket_fd;

    char *command = NULL;
    char *Args = NULL;
    split_input(commander_input, &command, &Args);
    
    if(command == NULL){
        printf("An error occurred!\n");
        free(args);
        return -1;
    }

    args->command = malloc(strlen(command) + 1);  
    if(args->command == NULL){
        perror("malloc");
        free(command);
        free(args);
        return -1;
    }
    strcpy(args->command, command);

    if(Args != NULL){
        args->args = malloc(strlen(Args) + 1); 
        if(args->args == NULL){
            perror("malloc");
            free(command);
            free(args->command);
            free(args);
            return -1;
        }
        strcpy(args->args, Args);
    } else {
        args->args = NULL;
    }

    free(command);
    free(Args);
    

    //create the controller thread
    pthread_t tid;
    if(pthread_create(&tid, NULL, controller_thread, (void *)args) != 0){
        perror("pthread_create");
        free(args->args);
        free(args->command);
        free(args);
        return -1;
    }

    //wait for the controller thread
    if(Join_Thread(tid) == -1){
        return -1;
    }

    // printf("CONTROLLER THREAD ENDED\n");

    free(args->args);
    free(args->command);
    free(args);

    return 0;
}

//function that handles all the jobs that are left in the common buffer after the server has terminated
//it sends the according message to the jobCommanders
int Send_Mes_To_Not_Executed_Commanders(){

    char * message = "SERVER TERMINATED BEFORE EXECUTION";

    for(int i = 0; i < shared_data->buffer->occupied_buffer_spots; i++){

        Job_Info * info = shared_data->buffer->buffer[i];

        //send a message to the jobCommander that the server has terminated
        //so that the commander doesnt wait to read the job output
        int value = htonl(SERVER_TERM_BEFORE_EXECUTION_VALUE);
        if(send(info->clientSocket, &value, sizeof(int), 0) < 0){
            perror("send");
            return -1;
        }

        //send the above message that the commander is going to display
        if(send(info->clientSocket, message, strlen(message), 0) < 0){
            perror("send"); 
            return -1;
        }

        //delete the item from the buffer
        Delete_Item(shared_data);

    }

    return 0;
}



//main thread
void * main_thread(void * args){

    Server_Info * info = (Server_Info *)args;

    //initialize the shared data
    Init_Shared_Data(info->buffer_size);

    //init the mutexes, cond vars 
    Init_Mutexes_And_Cond_Vars();
    
    //run the main thread loop (server loop)

    //initialize the server socket
    Init_Server_Socket(info);


    //create and run the worker threads
    info->worker_threads = Create_Worker_Threads(info->thread_pool_size);

    //start the server loop
    while(info->loop_flag){

        //printf("Accepting Connections...\n");

        //wait for connections and accept them
        int new_socket;
        if((new_socket = Accept_Connections(info)) == -1) continue;

        //read the data from the jobCommander socket
        char * commander_input = Read_Data(new_socket);

        // printf("ACCEPTED CONNECTION\n");

        //handle the command by spawning a controller thread
        if(Spawn_Controller_Thread(commander_input, new_socket, info) == -1){
            printf("An error occured while create a controller thread!\n");
            continue;
        }

        free(commander_input);

        //( the worker threads are in the backround waiting for a job to come )
        //if the client types the exit command then wait for all the controller threads to terminate execution
        //and then cancel the worker threads as well and then terminate the server

    }

    //handle the jobs that are left in the common buffer
    if(Send_Mes_To_Not_Executed_Commanders() == -1){
        printf("An error occured!\n");
        return NULL;
    }

    //wait for the worker threads to finish
    Join_Worker_Threads(info->thread_pool_size, info);

    //destroy mutexes, cond vars
    Destroy_Mutexes_And_Cond_Vars();

    free(info);

    return NULL;

}

//worker thread
void * worker_thread() {

    //loop until the stop_worker_threads variable is set to true, 
    //meaning that the users has typed the exit command
    while(!stop_worker_threads){

        //lock the buffer mutex
        pthread_mutex_lock(&(shared_data->buffer_mutex));

        // wait for a job to be placed in the buffer or the concurrecy to change, or for the stop signal
        while((shared_data->buffer->occupied_buffer_spots == 0 || shared_data->concurrency_level >= shared_data->max_concurrency) && !stop_worker_threads){
            pthread_cond_wait(&(shared_data->nonempty), &(shared_data->buffer_mutex));
        }

        //if the user has typed the exit command then unlock the mutex and exit the thread loop
        if(stop_worker_threads){
            pthread_mutex_unlock(&(shared_data->buffer_mutex));
            break;
        }


        //obtain the job info
        Job_Info *job_info = Delete_Item(shared_data);
        
        //increment the active jobs counter
        shared_data->concurrency_level++;

        //unlock the buffer mutex
        pthread_mutex_unlock(&(shared_data->buffer_mutex));

        //execute the job and send the data back to the client (jobCommander)
        pid_t PID = Execute_Job(job_info->job, job_info->clientSocket);

        // printf("JOB EXECUTED\n");

        //send a message to the commander that sent the specific job that the job has been executed
        //so that the commander has to wait to read the job output
        int num_to_send = 2;
        num_to_send = htonl(num_to_send);
        if (send(job_info->clientSocket, &num_to_send, sizeof(int), 0) < 0) {
            perror("send");
            pthread_exit(0);
        }

        //send the job output back to the commander
        if (Send_Mes_To_Commander(PID, job_info->clientSocket) == -1) {
            printf("An error occurred while sending the job output back to the commander!\n");
            pthread_exit(0);
        }

        pthread_mutex_lock(&(shared_data->buffer_mutex));

        //decrement the active jobs counter
        shared_data->concurrency_level = shared_data->concurrency_level - 1;
        // Signal the condition variable in case other worker threads are waiting
        pthread_cond_broadcast(&(shared_data->nonempty));

        pthread_mutex_unlock(&(shared_data->buffer_mutex));
    }

    //printf("WORKER THREAD ENDED\n");
    return NULL;
}





//controller thread
void * controller_thread(void * args){

    Controller_Args * controller_args = (Controller_Args *)args;

    //identify the command and execute it 
    if(IsCommand(controller_args->command, ISSUE_COMMAND)){
        //printf("EXECUTING ISSUE COMMAND\n");
        if(Execute_Issue_Command(shared_data, controller_args->args, controller_args->clientSocket) == COMMAND_ERROR){
            printf("An error occured while executing the issueJob command!\n");
            pthread_exit(0);
        }
    }
    else if(IsCommand(controller_args->command, SET_CON_COMMAND)){

        int new_con = atoi(controller_args->args);

        if(Execute_SetCon_Command(shared_data, new_con, controller_args->clientSocket) == COMMAND_ERROR){
            printf("An error occured while executing the setConcurrency command!\n");
            pthread_exit(0);
        }
    }
    else if(IsCommand(controller_args->command, STOP_COMMAND)){

        if(Execute_Stop_Command(shared_data, controller_args->args, controller_args->clientSocket) == COMMAND_ERROR){
            printf("An error occured while executing the stop command!\n");
            pthread_exit(0);
        }

    }
    else if(IsCommand(controller_args->command, POLL_COMMAND)){

        pthread_mutex_lock(&shared_data->buffer_mutex);
        char ** job_array = malloc(sizeof(char) * shared_data->buffer->occupied_buffer_spots);
        if(job_array == NULL){
            perror("malloc");
            pthread_exit(0);
        }
        pthread_mutex_unlock(&shared_data->buffer_mutex);

        if(Execute_Poll_Command(shared_data, &job_array, controller_args->clientSocket) == COMMAND_ERROR){
            printf("An error occured while executing the poll command!\n");
            pthread_exit(0);
        }

    }   
    else if(IsCommand(controller_args->command, EXIT_COMMAND)){

        if(Execute_Exit_Command(controller_args->info, controller_args->clientSocket) == COMMAND_ERROR){
            printf("An error occured while executing the issueJob command!\n");
            pthread_exit(0);
        }

        //set the stop_worker_threads flag to true so that they stop their execution 
        pthread_mutex_lock(&(shared_data->buffer_mutex));
        stop_worker_threads = true;
        //send a signal to the worker threads
        pthread_cond_broadcast(&(shared_data->nonempty));
        pthread_mutex_unlock(&(shared_data->buffer_mutex));
    }

    return NULL;

}

