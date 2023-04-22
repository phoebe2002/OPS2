
// Created by Tsitsi Nyamutswa on 25/03/2023.
//gerally speaking the code does the following
//Create n child processes.
//Create pipes.
//Close unused descriptors.
//Initiate random numbers.
//Parent process awaits data on pipe R and prints them on the stdout, it terminates after all data is processed.
//Child process sends random char on R pipe and exits.

//TASK 2 STAGE 1- PIPES
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
//#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// MAX_BUFF must be in one byte range
#define MAX_BUFF 200

unsigned char TEMP_FAILURE_RETRY(ssize_t read) {
    return 0;
}

volatile sig_atomic_t last_signal = 0;

int sethandler(void (*f)(int), int sigNo) // this function sets the signal handler function f for the signal signo
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction)); // sets all bytes of act to 0
    act.sa_handler = f; //
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void sig_handler(int sig)
{
    last_signal = sig;
}

void sig_killme(int sig)
{
    if (rand() % 5 == 0)
        exit(EXIT_SUCCESS);
}

void sigchld_handler(int sig) //this is the handler for the child signal
{
    pid_t pid;
    for (;;) {
        pid = waitpid(0, NULL, WNOHANG); // here we check if the child processes have terminated
        // if a child process has terminated it reads it's status with waitpid and continues the loop to check for more terminated child processes\
        //it terminates if there are no more child processes terminated
        if (0 == pid)
            return;
        if (0 >= pid) {
            if (ECHILD == errno)
                return;
            ERR("waitpid:");
        }
    }
}

//void child_work(int fd, int R) //here we define the work to be done by the child.
//{
//    srand(getpid());
//    char c = 'a' + rand() % ('z' - 'a'); // the child generates a random letter between a-z
//    if (write(R, &c, 1) < 0) // the child writes this to the pipe using WRITE
//        ERR("write to R");
//}

//new child work funtion
//Overall, the child_work function is responsible for continuously reading characters from a pipe and writing back
// to another pipe a variable number of repetitions of the same character, determined randomly.
void child_work(int fd, int R)
{
    char c, buf[MAX_BUFF + 1];
    unsigned char s;
    srand(getpid());
    if (sethandler(sig_killme, SIGINT))
        ERR("Setting SIGINT handler in child");
    for (;;) {
        if (TEMP_FAILURE_RETRY(read(fd, &c, 1)) < 1) // we read a single chararcetr into c
            ERR("read");
        s = 1 + rand() % MAX_BUFF; //we generate a random number between 1 and mx buff which fills buf
        buf[0] = s; //first byte is set to the value of 's' in dicating how many times c is to be repeated
        memset(buf + 1, c, s);
        if (TEMP_FAILURE_RETRY(write(R, buf, s + 1)) < 0)
            ERR("write to R");
    }
}


//void parent_work(int n, int *fds, int R) //here we read the characters from the pipe and print them onto the screen
//{
//    char c;
//    int status;
//    srand(getpid());//this initialisation doesn't actually do anything
//    while ((status = read(R, &c, 1)) == 1) //repeatedly calling read to read a single character from the pipe and return 1 o success
//        printf("%c", c);
//    if (status < 0)
//        ERR("read from R"); // if we fail to read then
//    printf("\n");
//}



void parent_work(int n, int *fds, int R)
{
    unsigned char c;
    char buf[MAX_BUFF];
    int status, i;
    srand(getpid());
    if (sethandler(sig_handler, SIGINT))
        ERR("Setting SIGINT handler in parent");
    for (;;) {
        if (SIGINT == last_signal) {
            i = rand() % n;
            while (0 == fds[i % n] && i < 2 * n)
                i++;
            i %= n;
            if (fds[i]) {
                c = 'a' + rand() % ('z' - 'a');
                status = TEMP_FAILURE_RETRY(write(fds[i], &c, 1));
                if (status != 1) {
                    if (TEMP_FAILURE_RETRY(close(fds[i])))
                        ERR("close");
                    fds[i] = 0;
                }
            }
            last_signal = 0;
        }
        status = read(R, &c, 1);
        if (status < 0 && errno == EINTR)
            continue;
        if (status < 0)
            ERR("read header from R");
        if (0 == status)
            break;
        if (TEMP_FAILURE_RETRY(read(R, buf, c)) < c)
            ERR("read data from R");
        buf[(int)c] = 0;
        printf("\n%s\n", buf);
    }
}


//void create_children_and_pipes(int n, int *fds, int R)
//{
//    int tmpfd[2];
//    int max = n; // creates a maximum of n children
//    while (n) {
//        if (pipe(tmpfd)) //for each child we create a pipe. PIPE FUNCTION TAKES A FILE DESCRIPTOR
//            ERR("pipe");
//        switch (fork()) { //next we call fork to create the child process// WE CAN USE IT'S SUCCESS. IF IT RETURNS ZERO THEN...
//            case 0:
//                while (n < max)
//                    if (fds[n] && close(fds[n++])) // here we iterate from n to the max closing all the open file descriptors
//                        ERR("close");
//                free(fds); //here we then free all memory that was used nby the file descriptor array
//                if (close(tmpfd[1]))// next we close the write end of the pipe [1 IS ALWAYS THE WRITE END]
//                    ERR("close");
//                child_work(tmpfd[0], R);// we call the childwork function with generates the random character and writes it to the pipe
//                if (close(tmpfd[0]))//we then close the read end of the pipe after execution is complete
//                    ERR("close");
//                if (close(R))
//                    ERR("close");
//                exit(EXIT_SUCCESS);
//
//            case -1: // this is when we have an error creating the child
//                ERR("Fork:");
//        }
//        if (close(tmpfd[0])) //we then close the read end of the pipe
//            ERR("close");
//        fds[--n] = tmpfd[1]; //we store the write end in the file descriptor array at the current index
//    }
//}

//NEW VERSION
void create_children_and_pipes(int n, int *fds, int R)
{
    int tmpfd[2];
    int max = n;
    while (n) {
        if (pipe(tmpfd))
            ERR("pipe");
        switch (fork()) {
            case 0:
                while (n < max)
                    if (fds[n] && TEMP_FAILURE_RETRY(close(fds[n++])))
                        ERR("close");
                free(fds);
                if (TEMP_FAILURE_RETRY(close(tmpfd[1])))
                    ERR("close");
                child_work(tmpfd[0], R);
                if (TEMP_FAILURE_RETRY(close(tmpfd[0])))
                    ERR("close");
                if (TEMP_FAILURE_RETRY(close(R)))
                    ERR("close");
                exit(EXIT_SUCCESS);

            case -1:
                ERR("Fork:");
        }
        if (TEMP_FAILURE_RETRY(close(tmpfd[0])))
            ERR("close");
        fds[--n] = tmpfd[1];
    }
}


//SAY WE WANT TO SEND SOMETHING FROM THE CHILD TO THE PARENT

void child_to_parent()//I'm sending a message
{
    int tmpfd[2];  //end tmpfd[0] is read end and tmpfd[1] is the write end
    if(pipe(tmpfd) == -1)
    {
        printf("error opening file");
        return;
    }
    int id = fork();
    if(id == 0)
    {
        close(tmpfd[0]); // we close the end that we are not using
        int x;
        printf("Enter a number: ");
        scanf("%d",&x);
        write(tmpfd[1], &x, sizeof(int));
        close(tmpfd[1]); // we close this straight after use

    }
    else
    {
        close(tmpfd[1]);
        int y;
        read(tmpfd[0],&y,sizeof(int));
        close(tmpfd[0]);
        printf("The kid sent me this! %d",y);
    }
}


void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "0<n<=10 - number of children\n");
    exit(EXIT_FAILURE);
}

//int main(int argc, char **argv)
//{
//    int n, *fds, R[2];
//    if (2 != argc)
//        usage(argv[0]);
//    n = atoi(argv[1]);
//    if (n <= 0 || n > 10)
//        usage(argv[0]);
//    if (pipe(R))
//        ERR("pipe");
//    if (NULL == (fds = (int *)malloc(sizeof(int) * n)))
//        ERR("malloc");
//    if (sethandler(sigchld_handler, SIGCHLD))
//        ERR("Seting parent SIGCHLD:");
//    create_children_and_pipes(n, fds, R[1]);
//    if (close(R[1]))
//        ERR("close");
//    parent_work(n, fds, R[0]);
//    while (n--)
//        if (fds[n] && close(fds[n]))
//            ERR("close");
//    if (close(R[0]))
//        ERR("close");
//    free(fds);
//    return EXIT_SUCCESS;
//}

//NEW MAIN
int main(int argc, char **argv)
{
    int n, *fds, R[2];
    if (2 != argc)
        usage(argv[0]);
    n = atoi(argv[1]);
    if (n <= 0 || n > 10)
        usage(argv[0]);
    if (sethandler(SIG_IGN, SIGINT))
        ERR("Setting SIGINT handler");
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting SIGINT handler");
    if (sethandler(sigchld_handler, SIGCHLD))
        ERR("Setting parent SIGCHLD:");
    if (pipe(R))
        ERR("pipe");
    if (NULL == (fds = (int *)malloc(sizeof(int) * n)))
        ERR("malloc");
    create_children_and_pipes(n, fds, R[1]);
    if (TEMP_FAILURE_RETRY(close(R[1])))
        ERR("close");
    parent_work(n, fds, R[0]);
    while (n--)
        if (fds[n] && TEMP_FAILURE_RETRY(close(fds[n])))
            ERR("close");
    if (TEMP_FAILURE_RETRY(close(R[0])))
        ERR("close");
    free(fds);
    return EXIT_SUCCESS;
}

//EXTRA NOTES FOR PIPES

