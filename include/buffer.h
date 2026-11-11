#ifndef BUFFER_H
#define BUFFER_H

#include "jobinfo.h"

//buffer struct
typedef struct{

    int occupied_buffer_spots;
    int buffer_length;
    Job_Info ** buffer;

}Buffer;


//shared data struct
typedef struct{

    int max_concurrency;
    int concurrency_level;
    Buffer * buffer;
    int job_counter;
    int * file_array;

    pthread_mutex_t max_con_mutex;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t nonempty;
    pthread_cond_t nonfull;

}Shared_Data;


//function prototypes
Buffer * Create_Buffer(int N);
void Insert_Item(Shared_Data * data, Job_Info * info);
//void Delete_Item(Shared_Data * data);
Job_Info* Delete_Item(Shared_Data *data);
Job_Info * Delete_Specific_Item(char *jobID, Shared_Data *data);
//void Delete_Specific_Item(char * jobID, Shared_Data * data);
bool Search_Item(char * jobID, Shared_Data * data);
//void Shift_Items(Buffer * buffer);



#endif //BUFFER_H

