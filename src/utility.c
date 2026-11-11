
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include "utility.h"
#include "threads.h"
#include "Info.h"

//this file contains usefull utility functions

//utility function to check if a string is an integer
bool IsInt(char *string){

    int len = strlen(string);
    int count = 0;

    for(int i=0;i<len;i++){
        if(isdigit(string[i])) count++;
    }

    if(count == len) return true;
    return false;

}



//utility function to check if a string is in this format : 'job_XX' 
//XX is a non negative number without leading zeros
bool Check_Job_String(char * input){

    char * input_copy = strdup(input);
    if(input_copy == NULL){
        perror("strdup");
        return false;
    }

    char *token;
    char *job_prefix;
    char *job_number;

    //tokenize the input string using strtok
    token = strtok(input_copy, "_");

    if(token != NULL){

        job_prefix = strdup(token); //duplicate the token
        token = strtok(NULL, "_");

        if(token != NULL){
            job_number = strdup(token); //duplicate the second token
        }
        else{
            printf("Error: Invalid input format\n");
            return false;
        }
    }
    else{
        printf("Error: Invalid input format\n");
        return false;
    }

    bool flag1 = strcmp(job_prefix, "job_");
    bool flag2 = IsInt(job_number);

    // Free allocated memory
    free(input_copy);
    free(job_prefix);
    free(job_number);

    return (flag1 && flag2);
}




//utility function to create a string in the form of: job_XX (XX is the given number)
char * Create_Job_String(int num){

    static char result[20]; 
    sprintf(result, "job_%d", num);

    return result;

}



int extract_number(const char* str){
    //find the position of the first digit in the string
    const char* p = str;
    while (*p && !isdigit(*p)) {
        p++;
    }

    //convert the substring to an integer
    int number = atoi(p);
    return number;
}

// pid_t Extract_Pid_From_String(char * pipe_string){

//     char* num_str = pipe_string;

//     while(*num_str && !isdigit(*num_str)){
//         num_str++;
//     }  
    
//     return atoi(num_str);

// }


int Count_Arguments(char * str){

    if(str == NULL){
        return 0;
    }

    int count = 0;
    int in_substring = 0;

    while (*str != '\0') {
        if (!isspace(*str) && !in_substring) {
            in_substring = 1;
            count++;
        } else if (isspace(*str)) {
            in_substring = 0;
        }
        str++;
    }

    return count;
}

//function that parses a job string to extract its arguments 
//and returns an array e.g. ls -l, array[0] = "ls" , array[1] = "-l"
void Parse_Job(char *cmd, char **argv){
    char *token;
    int index = 0;

    //get the first token
    token = strtok(cmd, " ");
    
    // iterate over the other tokens
    while(token != NULL){
        argv[index++] = token;
        token = strtok(NULL, " ");
    }
    
    argv[index] = NULL; //null terminate the array of arguments
}


//utility function that concatenates the command line arguments by placing the space 
//character in between each arguments
char * concatenate_args(int argc, char *argv[]){

    if (argc < 5) {

        return NULL; 
    }

    //calculate the total length needed for the concatenated string
    int total_length = 0;
    for(int i = 4; i < argc; ++i){
        total_length += strlen(argv[i]) + 1; 
    }

    char *result = (char *)malloc(total_length);
    if(result == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //concatenate the arguments
    result[0] = '\0'; //initialize the result string

    for(int i = 4; i < argc; ++i){
        strcat(result, argv[i]);
        if (i < argc - 1) {
            strcat(result, " "); //add space between arguments
        }
    }

    return result;
}


bool IsCommand(char * input_string, char * cmp_string){

    if(strcmp(input_string, cmp_string) == 0){
        return true;
    }

    return false;

}

//function to create the worker threads
pthread_t * Create_Worker_Threads(int N){ 

    pthread_t *thread_array = malloc(sizeof(pthread_t) * N);
    if(thread_array == NULL){
        perror("malloc");
        return NULL;
    }

    for(int i = 0; i < N; i++){
        if(pthread_create(&thread_array[i], NULL, worker_thread, NULL) != 0){
            perror("pthread_create");
            free(thread_array); 
            exit(1);
        }
    }

    return thread_array;
}

// function to join the worker threads
void Join_Worker_Threads(int N, Server_Info * info){
    for (int i = 0; i < N; i++) {
        if (pthread_join(info->worker_threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }
}






int Find_Num_Of_Digits(pid_t pid){ return (floor(log10(pid)) + 1); }
int Find_Num_Digits(int n){ return (floor(log10(n)) + 1); }

//function to create the output file that is in the <pid>.output format where
//pid is the process id
int Create_Output_File(pid_t PID){

    //create the output file name
    char * filename = malloc(sizeof(char) * (Find_Num_Of_Digits(PID) + 8));
    if(filename == NULL){
        perror("malloc");
        return -1;
    }

    snprintf(filename, (Find_Num_Of_Digits(PID) + 8), "%d.output", PID);
        
    //open the output file
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0){
        perror("open");
        return -1;
    }
        
    //redirect stdout to the file
    if(dup2(fd, STDOUT_FILENO) < 0){
        perror("dup2");
        close(fd);
        return -1;
    }
        
    //close the file descriptor 
    close(fd);

    return 0;
}




//function to send the output of the job to the commader via the client socket
//it reads from the <pid>.output file and sends its data
int Send_Mes_To_Commander(pid_t PID, int clientSocket){
        
    //create the output file name
    char * filename = malloc(sizeof(char) * (Find_Num_Of_Digits(PID) + 8));
    if(filename == NULL){
        perror("malloc");
        return -1;
    }
    snprintf(filename, (Find_Num_Of_Digits(PID) + 8), "%d.output", PID);

    //open the output file for reading
    int fd = open(filename, O_RDONLY);
    if(fd < 0){
        perror("open");
        return -1;
    }

    //read and print the file contents
    char buffer[SOCKET_BUFFER_LENGTH];
    ssize_t bytes_read;
    while((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0){

        buffer[bytes_read] = '\0'; 

        //send message back to the commander using the client sockets
        if(send(clientSocket, buffer, bytes_read, MSG_NOSIGNAL) < 0){
            perror("send");
            return -1;
        }

    }

    close(fd);

    //delete the file
    if(remove(filename) != 0){
        perror("remove");
        return -1;
    }

    close(clientSocket);

    return 0;

}


//function to execute a command
pid_t Execute_Job(char * job, int clientSocket){

    if(job == NULL){
        printf("ERROR: Job is invalid!\n");
        return -1;
    }

    //fork a child
    pid_t pid = fork();

    if(pid == -1){
        perror("fork");
        return -1;
    }

    //child_process
    if(pid == 0){

        if(Create_Output_File(getpid()) == -1){
            printf("An error occured while creating the output file!\n");
            return -1;
        }

        char * argv[Count_Arguments(job)];
        Parse_Job(job, argv);

        // sleep(2);
        execvp(job, argv); 

        perror("execvp");

        return pid;

    }
    //parent process
    else{

        int status;
        waitpid(pid, &status, 0);
        //sleep(1); //simulate some sleeping time

    }

    return pid;

}


// function to create a thread
pthread_t Create_Thread(void *(*f)(), Server_Info * info){

    pthread_t tid;

    if(pthread_create(&tid, NULL, f, (void *)info) != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    return tid;
}

// function to join a thread
int Join_Thread(pthread_t tid){

    if(pthread_join(tid, NULL) != 0){
        perror("pthread_join");
        return -1;
    }
    
    return 0;
}


void split_input(const char *input, char **command, char **arguments){
    //find the first space
    const char *space_pos = strchr(input, ' ');

    if(space_pos != NULL){
        
        *command = (char *)malloc(space_pos - input + 1);
        if(*command == NULL){
            perror("malloc");
            return;
        }
        strncpy(*command, input, space_pos - input);
        (*command)[space_pos - input] = '\0';  

        //skip the space and copy the arguments part if available
        const char *args_pos = space_pos + 1;
        if (*args_pos != '\0') {
            *arguments = strdup(args_pos);
        } else {
            *arguments = NULL;
        }
    } else {
        //ff there is no space, the entire input is the command and arguments are NULL
        *command = strdup(input);
        *arguments = NULL;
    }
}


//function to split the input string into two parts
void split_string(const char *input, char **command, char **arguments){

    //find the first space character in the input string
    const char *space_pos = strchr(input, ' ');
    if(space_pos == NULL){
        //if no space is found, the entire input is considered the command
        *command = strdup(input);
        *arguments = NULL;
        return;
    }

    //calculate the lengths of the command and arguments
    size_t command_len = space_pos - input;
    size_t arguments_len = strlen(input) - command_len - 1;

    *command = (char *)malloc(command_len + 1);
    if(*command == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strncpy(*command, input, command_len);
    (*command)[command_len] = '\0';  

    *arguments = (char *)malloc(arguments_len + 1);
    if (*arguments == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strncpy(*arguments, space_pos + 1, arguments_len);
    (*arguments)[arguments_len] = '\0'; 

    return;
}

char * Create_Term_String(const char* base_string, const char* jobID){

    //find the position of the space in the base string
    const char* space_pos = strchr(base_string, ' ');
    if(space_pos == NULL){
        perror("malloc");
        return NULL;
    }

    //calculate lengths of the parts
    size_t prefix_len = space_pos - base_string;
    size_t suffix_len = strlen(space_pos + 1);
    size_t jobID_len = strlen(jobID);

    //calculate the length of the new string
    size_t new_string_len = prefix_len + 1 + jobID_len + 1 + suffix_len;

    char* result = (char*)malloc(new_string_len + 1); 
    if(result == NULL){
        perror("malloc");
        return NULL;
    }

    //construct the new string
    strncpy(result, base_string, prefix_len); 
    result[prefix_len] = '\0'; 
    strcat(result, " ");
    strcat(result, jobID);
    strcat(result, " ");
    strcat(result, space_pos + 1); 

    return result;
}

//function that given a jobID and a job string it returns this string "jobID , job"
char * Format_Job_Info(Job_Info * info){

    char * result = malloc(sizeof(char) * (strlen(info->jobID) + strlen(info->job) + 4));
    if(result == NULL){
        perror("malloc");
        return NULL;
    }

    snprintf(result, 300, "%s , %s", info->jobID, info->job);

    return result;

}
