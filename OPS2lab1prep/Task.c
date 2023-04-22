//
// Created by Tsitsi Nyamutswa on 30/03/2023.
//
// 2-process application --> parent + child
// bidirectional (two-way communication) --> 2 pipes required: 1st: child R, parent W, 2nd child W, parent R

//Parent starts the communication by passing argument to the child. --> parent W

//later, common procedure for both processes:
//1. remove random letter from the string
//2. print the letter with its process PID
//3. pass it back to the other process
//4. process which receives empty string "" --> terminate   other process --> detect broken link condition and exit


// STAGE 1 --
// 1. create  2 processes (parent and child)
// 2. create pipes
// 3. close unused descriptors
// 4. after each pipe and close function call print out (PID, function, descriptor number)


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


#define MSG_SIZE 1024


// void usage(char *name)
// {
//     fprintf(stderr, "USAGE: %s string\n", name);
//     exit(EXIT_FAILURE);
// }

int main(int argc, char **argv)
{
    int pipefd1[2], pipefd2[2]; //file descriptors for pipe1 and pipe2 (bidirectional communication)
    char msg_buf[MSG_SIZE]; // buffer for message

//pipe creation
    printf("([%d], %s)\n", getpid(), "Main: Creating pipe1");

    if (pipe(pipefd1) <0)
    {
        ERR("pipe(): "); // error handling for pipe1
        return 1;
    }

    printf("([%d], %s)\n", getpid(), "Main: Creating pipe2");

    if (pipe(pipefd2) <0)
    {
        ERR("pipe(): "); // error handling for pipe2
        return 1;
    }

// fork the process to create 2 child processes

    pid_t pid = fork();

    switch (pid)
    {
        case -1: // when error occurs
            ERR("fork(): ");
            break; //return 1;

        case 0: //when child process occurs
        {
            close(pipefd1[1]); // close write end of pipe1
            printf("([%d], %s, %d)\n", getpid(), "Child: Closing write end of pipe1", pipefd1[1]);
            close(pipefd2[0]); // close read end of pipe2
            printf("([%d], %s, %d)\n", getpid(), "Child: Closing read end of pipe2", pipefd2[0]);

            while(1)
            {
                //read message from pipe1
                int ret = read(pipefd1[0], msg_buf, MSG_SIZE);
                if(ret ==-1)
                {
                    ERR("Error reading from pipe1");
                    return 1;
                }
                else if(ret == 0)
                {
                    break; // end of input
                }

                // print received message
                //printf("[%d] Received: %s", getpid(), msg_buf);
                printf("([%d], %s, %d)\n", getpid(), "Child: Received message from pipe1", pipefd1[0]);
                printf("([%d], %s, %s)\n", getpid(), "Child: Message content", msg_buf);

                //send response through pipe2
                if(write(pipefd2[1], msg_buf, strlen(msg_buf)) == -1)
                {
                    ERR("Error writing to pipe2");
                    return 1;
                }
                printf("([%d], %s, %d)\n", getpid(), "Child: Sent response through pipe2", pipefd2[1]);
            }

            //closing descriptors
            close(pipefd1[0]);
            printf("([%d], %s, %d)\n", getpid(), "Child: Closing read end of pipe1", pipefd1[0]);
            close(pipefd2[1]);
            printf("(%d, %s, %d)\n", getpid(), "Child: Closing write end of pipe2", pipefd2[1]);
            break; //exit(0);
        }
        default: //when parent process occurs
        {
            close(pipefd1[0]); // close read end of pipe1
            printf("([%d], %s, %d)\n", getpid(), "Parent: Closing read end of pipe1", pipefd1[0]);
            close(pipefd2[1]); // close write end of pipe2
            printf("([%d], %s, %d)\n", getpid(), "Parent: Closing write end of pipe2", pipefd2[1]);

            while(1)
            {
                // get message from user input
                printf("Enter message: ");
                fgets(msg_buf, MSG_SIZE, stdin);
                if (feof(stdin))
                {
                    break; // end of input
                }

                // send message through pipe1
                if (write(pipefd1[1], msg_buf, strlen(msg_buf)) == -1)
                {
                    ERR("Error writing to pipe1");
                    return 1;
                }

                // Read response from pipe2
                int ret = read(pipefd2[0], msg_buf, MSG_SIZE);
                if (ret == -1)
                {
                    ERR("Error reading from pipe2");
                    return 1;
                } else if (ret == 0)
                {
                    break; // end of input
                }

                // Print received response
                printf("Response: %s", msg_buf);
            }
            close(pipefd1[1]);
            close(pipefd2[0]);
            break; //exit(0);
        }

    }
    return EXIT_SUCCESS;
}

/*
int main(int argc, char **argv)
{

int pipefd1[2], pipefd2[2]; //file descriptors for pipe1 and pipe2 (bidirectional communication)
char msg_buf[MSG_SIZE]; // buffer for message

//pipe creation

if (pipe(pipefd1) <0)
{
	ERR("pipe(): "); // error handling for pipe1
    return 1;
}

if (pipe(pipefd2) <0)
{
	ERR("pipe(): "); // error handling for pipe2
    return 1;
}

// fork the process to create 2 child processes

pid_t pid = fork();

switch (pid)
{
case -1: // when error occurs
ERR("fork(): ");
    break; //return 1;

case 0: //when child process occurs
{
    close(pipefd1[1]); // close write end of pipe1
    close(pipefd2[0]); // close read end of pipe2

    while(1)
    {
        //read message from pipe1
        int n = read(pipefd1[0], msg_buf, MSG_SIZE);
        if(n ==-1)
        {
            ERR("Error reading from pipe1");
            return 1;
        }
        else if(n == 0)
        {
            break; // end of input
        }

        // print received message
        printf("[%d] Received: %s", getpid(), msg_buf);

        //send response through pipe2
        if(write(pipefd2[1], msg_buf, strlen(msg_buf)) == -1)
        {
            ERR("Error writing to pipe2");
            return 1;
        }
    }

    //closing descriptors
    close(pipefd1[0]);
    close(pipefd2[1]);
    break; //exit(0);
}
default: //when parent process occurs
{
    close(pipefd1[0]); // close read end of pipe1
    close(pipefd2[1]); // close write end of pipe2
}

while(1)
{
    // get message from user input
    printf("Enter message: ");
    fgets(msg_buf, MSG_SIZE, stdin);
    if (feof(stdin))
    {
        break; // end of input
    }

    // send message through pipe1
    if (write(pipefd1[1], msg_buf, strlen(msg_buf)) == -1)
    {
        ERR("Error writing to pipe1");
        return 1;
    }

    // Read response from pipe2
    int n = read(pipefd2[0], msg_buf, MSG_SIZE);
    if (n == -1)
    {
        ERR("Error reading from pipe2");
        return 1;
    } else if (n == 0)
    {
        break; // end of input
    }

     // Print received response
    printf("Response: %s", msg_buf);
}

   close(pipefd1[1]);
    close(pipefd2[0]);
    break; //exit(0);
}

return EXIT_SUCCESS;
}

*/



