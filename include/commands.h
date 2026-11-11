#ifndef COMMANDS_H
#define COMMANDS_H

#include "buffer.h"
#include "Info.h"


typedef int command_status;


#define COMMAND_ERROR (-1)
#define COMMAND_OK (1)


#define ISSUE_COMMAND "issueJob"
#define STOP_COMMAND "stop"
#define POLL_COMMAND "poll"
#define SET_CON_COMMAND "setConcurrency"
#define EXIT_COMMAND "exit"

//return messages
#define ISSUE_COMMAND_MES "JOB SUBMITTED"
#define STOP_COMMAND_MES_1 "JOB REMOVED"
#define STOP_COMMAND_MES_2 "JOB NOT FOUND"
#define SET_CON_COMMAND_MES "CONCURRENCY SET TO "
#define EXIT_MESSAGE "SERVER TERMINATED BEFORE EXECUTION"

//function prototypes
command_status Execute_Issue_Command(Shared_Data * data, char * job, int clientSocket);
command_status Execute_SetCon_Command(Shared_Data * data, int new_con, int clientSocket);
command_status Execute_Stop_Command(Shared_Data * data, char * jobID, int clientSocket);
command_status Execute_Poll_Command(Shared_Data * data, char ***job_array, int clientSocket);
command_status Execute_Exit_Command(Server_Info * info, int clientSocket);


#endif // COMMANDS_H