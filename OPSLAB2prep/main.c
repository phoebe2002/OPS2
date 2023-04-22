// so essentially what is happening is we are playing bingo
// at the start a parent process creates n sub-processes between 1 and 100 and we also create two message queues, pout and pin.
//pout sends messages (random numbers between 0 and 9) to the children using a message q

#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t children_left = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo) // always the same
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void mq_handler(int sig, siginfo_t *info, void *p)
{
    mqd_t *pin; //initialise a queue
    uint8_t ni; // initialise the number we are sending through
    unsigned msg_prio;

    pin = (mqd_t *)info->si_value.sival_ptr; // here we then assign our created queue with whatever queue we are dealing with

    static struct sigevent not ;
    not .sigev_notify = SIGEV_SIGNAL;
    not .sigev_signo = SIGRTMIN;
    not .sigev_value.sival_ptr = pin;
    if (mq_notify(*pin, &not ) < 0)  //this handles the notification that a signal has arrived
        ERR("mq_notify");

    for (;;) {
        if (mq_receive(*pin, (char *)&ni, 1, &msg_prio) < 1) { // checks if it successfully received
            if (errno == EAGAIN) //checks if it could be because of an interrupt
                break;
            else
                ERR("mq_receive");
        }
        if (0 == msg_prio) // if we have lost and so priority is 0, then we print out failure messsage
            printf("MQ: got timeout from %d.\n", ni);
        else
            printf("MQ:%d is a bingo number!\n", ni); // on success BINGO you stil dont win shit
    }
}

//understood
void sigchld_handler(int sig, siginfo_t *s, void *p)
{
    pid_t pid;
    for (;;) {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0) {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
        children_left--;
    }
}

//understood
void child_work(int n, mqd_t pin, mqd_t pout)
{
    int life;
    uint8_t ni; //this can hold unsigned number from 0-255 which is 1 byte, it is more precise
    uint8_t my_bingo;
    srand(getpid());
    life = rand() % LIFE_SPAN + 1; //hmmm rand number between 1 and 10 being the number of numbers it'll read from pout
    my_bingo = (uint8_t)(rand() % MAX_NUM); // random number between 0 - 9 generated here as the bingo number
    while (life--) {
        if (TEMP_FAILURE_RETRY(mq_receive(pout, (char *)&ni, 1, NULL)) < 1) // this function allows a process to receive a message from queue
            //receiving from pout, putting that number into ni, 1 byte, NULL - could be an unsigned integer that holds priority
            ERR("mq_receive");
        printf("[%d] Received %d\n", getpid(), ni); // print out the number  it got
        if (my_bingo == ni) { // if the numbers are equal then issa bingo hahaha
            if (TEMP_FAILURE_RETRY(mq_send(pin, (const char *)&my_bingo, 1, 1))) //so we send back to the parent success message
                //send through pin, my_bingo number, 1 byte, 1st priority
                ERR("mq_send");
            return; // then we stop playing
        }
    }
    if (TEMP_FAILURE_RETRY(mq_send(pin, (const char *)&n, 1, 0))) // otherwise we have failed and we send the n number back in pin with priority 0
        ERR("mq_send");
}

//understood
void parent_work(mqd_t pout)
{
    srand(getpid());
    uint8_t ni;
    while (children_left) {
        ni = (uint8_t)(rand() % MAX_NUM); //this is a random number between 0-9
        if (TEMP_FAILURE_RETRY(mq_send(pout, (const char *)&ni, 1, 0))) // we send this number through pout to the kidsss
            ERR("mq_send");
        sleep(1); //numbers are sent at 1 minute intervals
    }
    printf("[PARENT] Terminates \n");
}

//understood
void create_children(int n, mqd_t pin, mqd_t pout)//***
{
    while (n-- > 0) {
        switch (fork()) { //create n children and here we also have the required error checks
            case 0:
                child_work(n, pin, pout);
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
        children_left++; // here we increase the number of kids we have as we are adding them
    }
}

void usage(void)
{
    fprintf(stderr, "USAGE: signals n k p l\n");
    fprintf(stderr, "100 >n > 0 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int n;
    if (argc != 2) // this is the count of the number of arguments
        usage();
    n = atoi(argv[1]); // converts second argument to int and we check if this number is between 1-100
    if (n <= 0 || n >= 100)
        usage();

    mqd_t pin, pout; //initialise queues
    struct mq_attr attr; //initialise their attributes
    attr.mq_maxmsg = 10;  //set max number of messages sent through the queue be 10
    attr.mq_msgsize = 1; //each has a byte size of 1
    if ((pin = TEMP_FAILURE_RETRY(mq_open("/bingo_in", O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1) // queue opened for both reading and writing
        ERR("mq open in"); // here we open the pin queue
    if ((pout = TEMP_FAILURE_RETRY(mq_open("/bingo_out", O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open out");

    //general way in which our signals will be handled when we have a specific signal

    sethandler(sigchld_handler, SIGCHLD);
    sethandler(mq_handler, SIGRTMIN);
    create_children(n, pin, pout); //here we create our children and the queues are used in the childwork method

    static struct sigevent noti; // handling is done first before we request another signal
    noti.sigev_notify = SIGEV_SIGNAL; // generate signal when queue is not empty
    noti.sigev_signo = SIGRTMIN; //realtime signal
    noti.sigev_value.sival_ptr = &pin; //set to a pointer to pin descriptor specifying the message queue to be monitored when the notify happens
    if (mq_notify(pin, &noti) < 0) //then we do the notification
        ERR("mq_notify");
//essentially when a message is available in the pin ( put by the kid) we generate a signal (SIGRTMIN),. This is caught and handled by mq handler

    parent_work(pout); // parent sends messages every second to the children and they are handled accordingly

    mq_close(pin); //close and unlink queues
    mq_close(pout);
    if (mq_unlink("/bingo_in"))
        ERR("mq unlink");
    if (mq_unlink("/bingo_out"))
        ERR("mq unlink");
    return EXIT_SUCCESS;
}