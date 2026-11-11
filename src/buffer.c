

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "jobinfo.h"
#include "utility.h"
#include "buffer.h"


//the buffer acts like a queue, it behaves in a FIFO(first in first out) way

//function to create a buffer object
Buffer * Create_Buffer(int N){

    Buffer * buffer = malloc(sizeof(Buffer));
    if(buffer == NULL){
        perror("malloc");
        return NULL;
    }

    buffer->buffer_length = N;
    buffer->occupied_buffer_spots = 0;

    buffer->buffer = malloc(sizeof(Job_Info) * N);
    if(buffer->buffer == NULL){
        perror("malloc");
        return NULL;
    }

    for(int i = 0 ; i < N ; i++){
        buffer->buffer[i] = NULL;
    }

    return buffer;
}

//function to insert an item at the end of the buffer
void Insert_Item(Shared_Data * data, Job_Info * info){

    data->buffer->buffer[data->buffer->occupied_buffer_spots] = info;
    data->buffer->occupied_buffer_spots++;

    return;

}

//utility function that rearranges the array/buffer after a deletion
//shifts all the elements one spot to the left
void Shift_Items(Buffer * buffer){

    for(int i = 0; i < buffer->buffer_length - 1; i++){ 
        buffer->buffer[i] = buffer->buffer[i+1];
    }

    buffer->occupied_buffer_spots--;

    for(int i = buffer->occupied_buffer_spots ; i < buffer->buffer_length ; i++){
        buffer->buffer[i] = NULL;
    }

    return;
}

//function to delete a buffer item from the front of the buffer
Job_Info * Delete_Item(Shared_Data *data){ 

    Job_Info *deleted_item = NULL;

    //get the item to delete
    deleted_item = data->buffer->buffer[0];

    //shift items in the buffer
    Shift_Items(data->buffer);

    return deleted_item;
}



//function to delete a specific item from the buffer that has jobID equal to the given jobID
Job_Info * Delete_Specific_Item(char *jobID, Shared_Data *data){

    Job_Info * deleted_job = NULL;

    int index_to_delete = -1;

    //search for the jobID in the buffer
    for(int i = 0; i < data->buffer->occupied_buffer_spots; i++){
        if(strcmp(data->buffer->buffer[i]->jobID, jobID) == 0){
            index_to_delete = i;
            break;
        }
    }

    //if jobID was found, delete it and shift the remaining items
    if(index_to_delete != -1){
        deleted_job = data->buffer->buffer[index_to_delete];

        //shift items to fill the gap
        for(int i = index_to_delete; i < data->buffer->occupied_buffer_spots - 1; i++){
            data->buffer->buffer[i] = data->buffer->buffer[i + 1];
        }

        data->buffer->occupied_buffer_spots--;

        //clear the last element in the buffer
        data->buffer->buffer[data->buffer->occupied_buffer_spots] = NULL;
    }

    return deleted_job;
}


//function that searches for an item in the buffer
//returns true if the item exists in the buffer and false if not
bool Search_Item(char * jobID, Shared_Data * data){

    //if the buffer is empty return false
    if(data->buffer->occupied_buffer_spots == 0){
        return false;
    }

    for(int i = 0; i < data->buffer->buffer_length; i++){
        if(strcmp(jobID, data->buffer->buffer[i]->jobID) == 0 ){
            return true;
        }
    }

    return false;
}