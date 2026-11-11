#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>



#include "utility.h"
#include "buffer.h"
#include "Info.h"
#include "commands.h"






//function to execute the issueJob command
command_status Execute_Issue_Command(Shared_Data * data, char * job, int clientSocket){

    //get accesss to the shared buffer 
    //create the job string <>
    //add the new job to the shared buffer

    //lock the buffer mutex
    pthread_mutex_lock(&(data->buffer_mutex)); 

    Job_Info *new_job = malloc(sizeof(Job_Info));
    if(new_job == NULL){
        perror("malloc");
        return COMMAND_ERROR;
    }

    //create the jobID string 
    new_job->jobID = Create_Job_String(data->job_counter);

    if(new_job == NULL){
        return COMMAND_ERROR;
    }

    //increase the job counter
    data->job_counter++;

    new_job->job = malloc(sizeof(char) * strlen(job));
    if(new_job->job == NULL){
        perror("malloc");
        return COMMAND_ERROR;
    }

    new_job->clientSocket = clientSocket;

    strcpy(new_job->job, job);

    //suspend the process if the buffer is full
    //the controller thread cannot add to the buffer if its full
    while(data->buffer->occupied_buffer_spots == data->buffer->buffer_length ){

        //wait on the condition
        pthread_cond_wait(&(data->nonfull), &(data->buffer_mutex));
        
    }

    //insert the job to the common buffer
    Insert_Item(data, new_job);

    //signal all the worker threads that are waiting for an item to be inserted in the buffer
    pthread_cond_broadcast(&(data->nonempty));

    //unlock the buffer mutex
    pthread_mutex_unlock(&(data->buffer_mutex)); 

    //create the message to return to the jobCommander
    char * return_message = Create_Term_String(ISSUE_COMMAND_MES, new_job->jobID);
    if(return_message == NULL){
        return COMMAND_ERROR;
    }

    //send the message
    if(send(clientSocket, return_message, strlen(return_message), MSG_NOSIGNAL) < 0){
        perror("send");
        return COMMAND_ERROR;
    }

    free(return_message);

    return COMMAND_OK;
}


//function to execute the stop command
command_status Execute_Stop_Command(Shared_Data * data, char * jobID, int clientSocket){

    char * return_message;

    //lock the buffer mutex
    pthread_mutex_lock(&(data->buffer_mutex));

    //search if the job with jobID exists in the common buffer
    if(Search_Item(jobID, data)){

        //if exists delete the job with jobID
        Delete_Specific_Item(jobID, data);

        //signal/notify all the controller threads that an item has been deleted from the buffer
        //so a thread can add an their item to the buffer if the thread was waiting cause the buffer was full
        pthread_cond_broadcast(&(data->nonfull));

        //create the message to send back to the jobCommander
        return_message = Create_Term_String(STOP_COMMAND_MES_1, jobID);
        if(return_message == NULL){
            return COMMAND_ERROR;
        }

        //send the message
        if(send(clientSocket, return_message, strlen(return_message), MSG_NOSIGNAL) < 0){
            perror("send");
            return COMMAND_ERROR;
        }

        //send a message to the jobCommander that the job they added to the server has stoped
        //so they dont wait to read the job output 
        int num_to_send = htonl(1);
        if (send(clientSocket, &num_to_send, sizeof(int), 0) < 0) {
            perror("send");
            return COMMAND_ERROR;
        }
    }
    else{

        //if the job doesnt exist in the buffer then send the according message

        return_message = Create_Term_String(STOP_COMMAND_MES_2, jobID);
        if(return_message == NULL){
            return COMMAND_ERROR;
        }

        if(send(clientSocket, return_message, strlen(return_message), MSG_NOSIGNAL) < 0){
            perror("send");
            return COMMAND_ERROR;
        }

        free(return_message);
    }

    close(clientSocket);

    //unlock the buffer mutex
    pthread_mutex_unlock(&(data->buffer_mutex));

    return COMMAND_OK;
}   


//function that sends an array of the job information that exists in the buffer
//waiting to be executed
int Send_Data_Poll(int socket, int num_strings, char ** strings){

    //send the number of strings
    int num_strings_network_order = htonl(num_strings);
    if(send(socket, &num_strings_network_order, sizeof(int), 0) < 0){
        perror("send");
        return -1;
    }

    for(int i = 0; i < num_strings; i++){

        int str_len = strlen(strings[i]);
        int str_len_network_order = htonl(str_len);

        //send the length of each string
        if(send(socket, &str_len_network_order, sizeof(int), 0) < 0){
            perror("send");
            return -1;
        }

        //send the string
        if(send(socket, strings[i], str_len, 0) < 0){
            perror("send");
            return -1;
        }
    }

    close(socket);

    return 0;

}

//function that executes the poll command
command_status Execute_Poll_Command(Shared_Data * data, char ***job_array, int clientSocket){

    bool flag = false;
    int n = 0;

    //lock the buffer mutex
    pthread_mutex_lock(&(data->buffer_mutex));

    //if the buffer is empty
    if(data->buffer->occupied_buffer_spots == 0){
        flag = true;
    }
    //if the buffer is not empty
    else{

        //fill in the array that is going to be returned to the commander
        for(int i = 0; i < data->buffer->occupied_buffer_spots ; i++){

            char * job_string = Format_Job_Info(data->buffer->buffer[i]);
            if(job_string == NULL){
                return COMMAND_ERROR;
            }

            (*job_array[i]) = malloc(sizeof(char) * strlen(job_string));
            if((*job_array[i]) == NULL){
                perror("malloc");
                return COMMAND_ERROR;
            }

            (*job_array[i]) = job_string;
        }

    }

    n = data->buffer->occupied_buffer_spots;

    //unlock the buffer mutex
    pthread_mutex_unlock(&(data->buffer_mutex));

    //if the buffer is empty
    if(flag){
        char * message = "THE BUFFER IS EMPTY";
        if(send(clientSocket, message, strlen(message), MSG_NOSIGNAL) < 0){
            perror("send");
            return COMMAND_ERROR;
        }
    }
    //else send the array of jobs 
    else{

        if(Send_Data_Poll(clientSocket, n, (*job_array)) == -1){
            return COMMAND_ERROR;
        }

    }

    return COMMAND_OK;
}

//function to execute the setConcurrency command
command_status Execute_SetCon_Command(Shared_Data * data, int new_concurrency, int clientSocket){

    //lock the buffer mutex
    pthread_mutex_lock(&(data->buffer_mutex));

    //obtain the previous max concurrency
    int old_con = data->max_concurrency;

    //update the concurrency
    data->max_concurrency = new_concurrency;

    //signal/broadcast if the new concurrency is larger than the old
    //so that if there are worker threads available they can execute the jobs waiting in the buffer
    if(old_con < new_concurrency){
        pthread_cond_broadcast(&(data->nonempty));
    }

    //unlock the buffer mutex
    pthread_mutex_unlock(&(data->buffer_mutex));
    
    //construct the return message
    char * return_message = malloc(sizeof(char) * (strlen(SET_CON_COMMAND_MES) + Find_Num_Digits(new_concurrency) + 1));
    if(return_message == NULL){
        perror("malloc");
        return COMMAND_ERROR;
    }

    snprintf(return_message, strlen(SET_CON_COMMAND_MES) + Find_Num_Digits(new_concurrency) + 1, "CONCURRENCY SET TO %d", new_concurrency);

    //send the return message back to the jobCommander
    if(send(clientSocket, return_message, strlen(return_message), MSG_NOSIGNAL) < 0){
        perror("send");
        return COMMAND_ERROR;
    }

    return COMMAND_OK;
}

//function that executes the exit command
command_status Execute_Exit_Command(Server_Info * info, int clientSocket){

    //update the loop flag to false so that the server stops accepting new connections
    info->loop_flag = false;

    //construct and send the message back to the commander that the server has terminated
    char * return_message = "SERVER TERMINATED";
        
    if(send(clientSocket, return_message, strlen(return_message), MSG_NOSIGNAL) < 0){
        perror("send");
        return COMMAND_ERROR;
    }

    return COMMAND_OK;
}