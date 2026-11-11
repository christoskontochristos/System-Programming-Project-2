#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Info.h"
#include "jobinfo.h"

#define JOB_STOPED_VALUE (1)
#define JOB_EXECUTE_VALUE (2)
#define SERVER_TERM_BEFORE_EXECUTION_VALUE (3)

#define OUTPUT_FILE_LAYOUT ".output"

#define BUFFER_LENGTH 128
#define SOCKET_BUFFER_LENGTH 1024

#define DEFAULT_CONCURRECNY 1

//function prototypes
bool IsInt(char *string);
bool Check_Job_String(char * input);
char * Create_Job_String(int num);
int extract_number(const char* str);
pid_t Extract_Pid_From_String(char * pipe_string);
int Count_Arguments(char *cmd);
void Parse_Job(char *cmd, char **argv);
char * concatenate_args(int argc, char *argv[]);
bool IsCommand(char * input_string, char * cmp_string);
pthread_t *Create_Worker_Threads(int N);
void Join_Worker_Threads(int N, Server_Info * info);
pid_t Execute_Job(char * job, int clientSocket);
pthread_t Create_Thread(void *(*f)(), Server_Info * info);
int Join_Thread(pthread_t tid);
int Create_Output_File(pid_t PID);
void split_input(const char *input, char **command, char **arguments);
void split_string(const char *input, char **command, char **arguments);
char * Create_Term_String(const char* base_string, const char* jobID);
char * Format_Job_Info(Job_Info * info);
int Find_Num_Digits(int n);
int Send_Mes_To_Commander(pid_t PID, int clientSocket);

#endif // UTILITY_H