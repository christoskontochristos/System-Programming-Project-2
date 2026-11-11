#ifndef JOBINFO_H
#define JOBINFO_H

//the job info struct holds all the job information that is needed
typedef struct{ 

    char * job;
    char * jobID;
    int clientSocket;

}Job_Info;


#endif // JOBINFO_H