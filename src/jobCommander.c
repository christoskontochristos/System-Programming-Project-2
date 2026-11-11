#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>


#include "utility.h"
#include "commands.h"
#include "Info.h"




//function that parses the command line arguments of the jobCommander
//and checks if its a valid command
bool Check_Valid_Command(int counter, Commander_Info * info){

    if(strcmp(info->command, ISSUE_COMMAND) == 0){

        if(counter < 1){
            printf("ERROR: The 'issueJob' needs more arguments!\n");
            return false;
        }

    }
    else if(strcmp(info->command, SET_CON_COMMAND) == 0){

        if(counter != 1){
            printf("ERROR: The 'setConcurrency' command takes exactly one argument (an integer)!\n");
            return false;
        }
        else{
            if(!IsInt(info->command_args)){
                printf("ERROR: The argument of the 'setConcurrency' must be an integer!\n");
                return false;
            }
        }

    }
    else if(strcmp(info->command, POLL_COMMAND) == 0){

        if(counter != 0){
            printf("ERROR: The 'poll' command does not take any arguments!\n");
            return false;
        }

    }
    else if(strcmp(info->command, STOP_COMMAND) == 0){

        if(counter != 1){
            printf("ERROR: The 'stop' command takes one argument!\n");
            return false;
        }
        else{

            if(!Check_Job_String(info->command_args)){
                printf("ERROR: The argument of the 'stop' command has to be string in the form of : job_XX (XX is a non negative number)\n");
                return false;
            }
        }

    }
    else if(strcmp(info->command, EXIT_COMMAND) == 0){

        if(counter != 0){
            printf("ERROR: The 'exit' command does not take any arguments!\n");
            return false;
        }

    }
    else{
        printf("Not a valid command!\n");
        return false;
    }

    return true;

}

//
bool Parse_Args(int argc, char *argv[]){

    if(argc < 3){
        printf("Too few command line arguments given in the jobCommander!\n");
        return false;
    }

    if(!IsInt(argv[2])){
        printf("The second argument must be an integer (the port number)!\n");
        return false;
    }

    return true;

}


bool Init_Commander(Commander_Info *info, int argc, char * argv[]){

    info->SERVER_HOST_ADDR = malloc(strlen(argv[1]) + 1);
    if(info->SERVER_HOST_ADDR == NULL){
        perror("malloc");
        return false;
    }

    strcpy(info->SERVER_HOST_ADDR, argv[1]);

    info->PORT_NUM = atoi(argv[2]);

    info->command = malloc(strlen(argv[3]) + 1);
    if (info->command == NULL) {
        perror("malloc");
        return false;
    }

    strcpy(info->command, argv[3]);

    char * command_args = concatenate_args(argc, argv);
    if(command_args == NULL){
        //return false;
        info->command_args = NULL;
        return true;
    }

    info->command_args = malloc(strlen(command_args) + 1);
    if (info->command_args == NULL) {
        perror("malloc");
        return false;
    }

    strcpy(info->command_args, command_args);

    //printf("SERVER_HOST_ADDR: %s\n", info->SERVER_HOST_ADDR);

    return true;
}

//function to connect to the server
int Connect_To_Server(Commander_Info * info){

    int network_socket;

    //create a stream socket
    network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(network_socket < 0){
        perror("socket");
        return -1;
    }

    //initialize port number and address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    // server_address.sin_addr.s_addr = info->SERVER_HOST_ADDR;
    server_address.sin_port = htons(info->PORT_NUM);

    // printf("%s\n", info->SERVER_HOST_ADDR);

    //use gethostbyname 
    struct hostent *rem;
    if((rem = gethostbyname(info->SERVER_HOST_ADDR)) == NULL){ 
        herror("gethostbyname"); 
        exit(1);
    }
    memcpy(&server_address.sin_addr, rem->h_addr, rem->h_length);

    //initiate a socket connection
    int connection_status = connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if(connection_status < 0){
        perror("connect");
        close(network_socket);
        return -1;
    }

    return network_socket;
}


//function to send the data to the server/ the message the user typed
int Send_Data_To_Server(int socket_fd, Commander_Info *info) {

    size_t buffer_size;
    char * buffer;

    if(info->command_args == NULL){
        //calculate the size of the buffer
        buffer_size = strlen(info->command) + 2; // 1 for space and 1 for null terminator

        //allocate memeory for the buffer
        buffer = malloc(buffer_size);
        if (buffer == NULL) {
            perror("malloc");
            return -1;
        }

        //initialize the buffer and construct the message
        snprintf(buffer, buffer_size, "%s", info->command);

        //printf("buffer to send : %s\n", buffer);
    }
    else{

        //calculate the size of the buffer
        buffer_size = strlen(info->command) + strlen(info->command_args) + 2; 

        //allocate the buffer
        buffer = malloc(buffer_size);
        if (buffer == NULL) {
            perror("malloc");
            return -1;
        }

        //initialize the buffer and construct the message
        snprintf(buffer, buffer_size, "%s %s", info->command, info->command_args);

        //printf("buffer to send : %s\n", buffer);

    }


    //send the data
    if (send(socket_fd, buffer, buffer_size - 1, 0) == -1) { // Exclude null terminator in send size
        perror("send");
        free(buffer);
        return -1;
    }

    // close(socket_fd);

    free(buffer);
    return 0;
}


//function that reads the job output from the socket sent by the server 
//after the job has executed
int Receive_Job_Output(int clientSocket){

    char buffer[SOCKET_BUFFER_LENGTH];
    ssize_t bytes_received;

    //read from the socket 
    while((bytes_received = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0){

        buffer[bytes_received] = '\0'; // Null-terminate the buffer for printing

        printf("%s", buffer);
    }

    if(bytes_received < 0){
        perror("recv");
        return -1;
    }

    close(clientSocket);

    return 0;
}


//function to read the return message from the server 
int Read_Data_From_Server(int clientSocket){

    char message[BUFFER_LENGTH];
    int bytes_received = recv(clientSocket, message, sizeof(message) - 1, 0);

    if(bytes_received <= 0){
        if(bytes_received == 0){
            // printf("reading done!\n");
        }
        else{
            perror("recv");
        }
        return -1;
    }

    message[bytes_received] = '\0'; // null terminate the received data

    printf("%s\n", message);
    return 0;
}

//function to read the return message from the server if its the poll command
int Read_Data_From_Server_Poll(int socket, char ***strings, int *num_strings){

    //receive the number of strings
    if(recv(socket, num_strings, sizeof(int), 0) <= 0){
        perror("recv");
        return -1;
    }

    *num_strings = ntohl(*num_strings);

    //allocate memory for the array of strings
    *strings = (char **)malloc(*num_strings * sizeof(char *));
    if(*strings == NULL){
        perror("malloc");
        return -1;
    }

    for (int i = 0; i < *num_strings; i++) {
        int str_len;

        //rceive the length of each string
        if(recv(socket, &str_len, sizeof(int), 0) <= 0){
            perror("recv");
            return -1;
        }
        str_len = ntohl(str_len);

        //allocate memory for the string
        (*strings)[i] = (char *)malloc((str_len + 1) * sizeof(char));
        if((*strings)[i] == NULL){
            perror("malloc");
            return -1;
        }

        //receive the string
        if(recv(socket, (*strings)[i], str_len, 0) <= 0){
            perror("recv");
            return -1;
        }

        (*strings)[i][str_len] = '\0'; //null terminate the string
    }

    return 0;
}

//function to run the jobCommander
int Run_Commander(int argc, char * argv[]){

    //parse the commander command line arguments
    if(!Parse_Args(argc, argv)){
        return 1;
    }

    Commander_Info *info = malloc(sizeof(Commander_Info));
    if(info == NULL){
        perror("malloc");
        return 1;
    }

    //initialize the commander
    if(!Init_Commander(info, argc, argv)){
        free(info);
        return 1;
    }

    //check for the command's validity
    if(!Check_Valid_Command(Count_Arguments(info->command_args), info)){
        free(info);
        return 0;
    }

    //create a socket a send the info to the server
    //check for valid connection 
    //return error message if the connection fails 

    int net_socket;

    //connect to the server
    if((net_socket = Connect_To_Server(info)) == -1){
        printf("An error occured while connecting to the server!\n");
        return 1;
    }

    //send the data to the server
    if(Send_Data_To_Server(net_socket, info) == -1){
        printf("An error occured while sending the data to the server!\n");
        return 1;
    }


    //wait to get the message back from the server
    //here the commander stays in blocked state until there is something to read from the socket

    //if the user typed the issueJob command
    if(strcmp(info->command, ISSUE_COMMAND) == 0){

        //read the "JOB <jobID> SUBMITTED" message
        if(Read_Data_From_Server(net_socket) == -1){
            printf("An error occured while reading the data from the server!\n");
            return 1;
        }

        //check if the job has been stoped by another commander
        int buffer;
        if (recv(net_socket, &buffer, sizeof(int), 0) < 0) {
            perror("recv");
            return 1;
        }

        // Convert from network byte order to host byte order
        int value = ntohl(buffer);

        // printf("%d\n", value);
        //if the job has been stoped by another commander then dont wait for a message
        if(value == JOB_STOPED_VALUE){
            //do nothing
        }        
        //if the job has been executed then read the job output from the server 
        else if(value == JOB_EXECUTE_VALUE){
            if(Receive_Job_Output(net_socket) == -1){
                printf("An error occured while reading the data from the server!\n");
                return 1;
            }
        }
        //if the server terminated before the job has executed 
        //read the "SERVER TERMINATED BEFORE EXECUTION" message
        else if(value == SERVER_TERM_BEFORE_EXECUTION_VALUE){
            char buffer[BUFFER_LENGTH];
            if(recv(net_socket, &buffer, strlen(buffer), 0) < 0){
                perror("recv");
                return 1;
            }
            printf("%s\n", buffer);
        }

    }
    else{

        //if the command the user typed is the poll command
        if(strcmp(info->command, POLL_COMMAND) == 0){

            char **job_array;
            int str_num;

            //use the specific reading function to read the server message
            //(the array of jobs that are in the buffer)
            if(Read_Data_From_Server_Poll(net_socket, &job_array, &str_num) == -1){
                printf("An error occured while reading the data from the server (poll)!\n");
                return 1;
            }

            //print the message
            for(int i = 0 ; i < str_num ; i++){ printf("%s\n", job_array[i]); }

        }
        else{
            
            //if its any of the other commands read the according message
            if(Read_Data_From_Server(net_socket) == -1){
                printf("An error occured while reading the data from the server!\n");
                return 1;
            }

        }

    }

    close(net_socket);
    free(info);
    return 0;
}


//main function
int main(int argc, char * argv[]){

    if(Run_Commander(argc, argv) == -1){
        return 1;
    }

    return 0;

}



// //main function
// int main(int argc, char *argv[]){

//     //parse the commander command line arguments
//     if(!Parse_Args(argc, argv)){
//         return 1;
//     }

//     Commander_Info *info = malloc(sizeof(Commander_Info));
//     if(info == NULL){
//         perror("malloc");
//         return 1;
//     }

//     //initialize the commander
//     if(!Init_Commander(info, argc, argv)){
//         free(info);
//         return 1;
//     }

//     //check for the command's validity
//     if(!Check_Valid_Command(Count_Arguments(info->command_args), info)){
//         free(info);
//         return 0;
//     }

//     //create a socket a send the info to the server
//     //check for valid connection 
//     //return error message if the connection fails 

//     int net_socket;

//     if((net_socket = Connect_To_Server(info)) == -1){
//         printf("An error occured while connecting to the server!\n");
//         return 1;
//     }

//     if(Send_Data_To_Server(net_socket, info) == -1){
//         printf("An error occured while sending the data to the server!\n");
//         return 1;
//     }


//     //wait to get the message back from the server
//     //here the commander stays in blocked state until there is something to read from the socket
//     if(strcmp(info->command, ISSUE_COMMAND) == 0){

//         if(Read_Data_From_Server(net_socket) == -1){
//             printf("An error occured while reading the data from the server!\n");
//             return 1;
//         }

//         //check if the job has been stoped by another commander
//         int value;
//         int buffer;
//         if((value = recv(net_socket, &buffer, sizeof(int), 0)) < 0){
//             perror("recv");
//             return 1;
//         }

//         // printf("value : %d\n", value);

//         if(value == 1){
//             //do nothing
//         }        
//         else{
//             if(Receive_Job_Output(net_socket) == -1){
//                 printf("An error occured while reading the data from the server!\n");
//                 return 1;
//             }
//         }

//     }
//     else{

//         if(strcmp(info->command, POLL_COMMAND) == 0){

//             char **job_array;
//             int str_num;

//             if(Read_Data_From_Server_Poll(net_socket, &job_array, &str_num) == -1){
//                 printf("An error occured while reading the data from the server (poll)!\n");
//                 return 1;
//             }

//             for(int i = 0 ; i < str_num ; i++){ printf("%s\n", job_array[i]); }

//         }
//         else{

//             if(Read_Data_From_Server(net_socket) == -1){
//                 printf("An error occured while reading the data from the server!\n");
//                 return 1;
//             }

//         }

//     }

//     close(net_socket);
//     free(info);

//     return 0;
// }