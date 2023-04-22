//
// Created by Tsitsi Nyamutswa on 30/03/2023.
//

#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define MAX_SIZE 50

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// STAGE 2 --
// Both processes send random substring of program argument (argv[1]) over the pipe,
// read from its pipe,
// print the incoming data and exit.
// At this stage you must be able to handle variable length messages.


// STAGE 3 --
// Parent process sends argument to the child, child sends it back unchanged and so on
// until C-c is pressed.
// At each cycle process should print the message and its PID

// STAGE 4 --
// Add letter reduction, STOP condition
// detect broken link to exit


void usage(char *name)
{
    fprintf(stderr, "USAGE: %s string\n", name);
    exit(EXIT_FAILURE);
}

void remove_random_letter(char *str, int len, int random_index)
{
    for (size_t i = random_index; i < len-1; i++)
    {
        str[i] = str[i + 1];
    }
    str[len-1] = '\0';
}

int main(int argc, char **argv)
{
    if (2 != argc)
    {
        usage(argv[0]);
    }
    char *argument = argv[1]; //program argument (argv[1])

    int length = strlen(argument); // strlen counts up to '/0' not including null termination char

    int pipefd1[2], pipefd2[2];
    int pid, ret;
    char msg_buf[MAX_SIZE]; // buffer for string

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

    printf("PID %d: %s\n", getpid(), argument);

    // fork the process to create 2 child processes
    pid_t pid = fork();

    switch (pid)
    {
        case -1: // when error occurs
            ERR("fork(): ");
            break; //return 1;

        case 0: //when child process occurs
        {
            int length, random;

            srand(getpid());
            if (close(pipefd1[1]) < 0) // closing pipe 1 fd for writing
                ERR("close");
            if (close(pipefd2[0]) < 0) // closing pipe 2 fd for reading
                ERR("close");

            while (1)
            {
                int i = 1, k = 0;
                if ((ret = read(pipefd1[0], msg_buf, MAX_SIZE)) < MAX_SIZE) //child reads pipe1
                {
                    ERR("Read");
                }
                else
                {

                    length = msg_buf[0]; // retriving the length of the message

                    if (length == 0) // empty string
                    {
                        return EXIT_SUCCESS; // process shall terminate immediately
                        //break;
                    }
                    else // removing random letter
                    {
                        random = rand() % length;  //int index = rand() % strlen(str); --> code for random letter
                        char tmp[length + 1];
                        while (msg_buf[i] != '\0')
                        {
                            tmp[k] = msg_buf[i];
                            i++;
                            k++;
                        }
                        /*
                        tmp[k] = '\0';
                        for(int i = 0, int k = 0; msg_buf[i] != '\0'; i++, k++)
                        {
                            tmp[k] = msg_buf[i];
                            i++;
                            k++;
                        }
                        */

                        remove_random_letter(tmp, length, random);
                        printf("PID %d: %s\n", getpid(), tmp);
                        length--; //reducing string length after letter removal

                        // recreating the buffer
                        msg_buf[0] = length; //establishing new msg buffer length
                        strcpy(msg_buf + 1, tmp);
                        memset(msg_buf + 1 + length + 1, 1, MAX_SIZE - 1 - length - 1); // resetting to const size

                        // sending buffer back to child
                        if ((ret = write(pipefd2[1], msg_buf, MAX_SIZE)) < MAX_SIZE) // child writes pipe2
                            ERR("Write");
                    }
                }
            }
        }
        default: //when parent process occurs
        {
            int random;
            srand(getpid());
            if (close(pipefd1[0]) < 0) // closing pipe 1 fd for reading
                ERR("close");
            if (close(pipefd2[1]) < 0) // closing pipe 2 fd for writing
                ERR("close");

            msg_buf[0] = length; // first element of the buffer stores length of the message
            strcpy(msg_buf + 1, argument);
            memset(msg_buf + 2 + length, 1, MAX_SIZE - length - 2); // message must have constant size

            // starting the communication between the processes
            if ((ret = write(pipefd1[1], msg_buf, MAX_SIZE)) < MAX_SIZE)
                ERR("Write");

            while (1)
            {
                int i = 1, k =0;
                if ((ret = read(pipefd2[0], msg_buf, MAX_SIZE)) < MAX_SIZE)
                {
                    ERR("Read");
                }
                else
                {
                    length = msg_buf[0]; // retriving the length of the message

                    if (length == 0) // if empty string
                    {
                        // closing file descriptors == killing the child
                        kill(pid,SIGKILL);
                        return EXIT_SUCCESS; //break;
                    }
                    else // removing random letter
                    {
                        random = rand() % length;
                        char temp[length + 1];
                        while (msg_buf[i] != '\0')
                        {
                            temp[k] = msg_buf[i];
                            i++;
                            k++;
                        }
                        temp[k] = '\0';

                        remove_random_letter(temp, length, random);
                        printf("PID %d: %s\n", getpid(), temp);
                        length--;

                        // recreating the buffer
                        msg_buf[0] = length;
                        strcpy(msg_buf + 1, temp);
                        memset(msg_buf + 1 + length + 1, 1, MAX_SIZE - 1 - length - 1); //resetting to const size
                        // sending buffer back to child
                        if ((ret = write(pipefd1[1], msg_buf, MAX_SIZE)) < MAX_SIZE)
                            ERR("Write");
                    }
                }
            }
        }
            return EXIT_SUCCESS;
    }