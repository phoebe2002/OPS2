//
// Created by Tsitsi Nyamutswa on 19/04/2023.
//
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

int main(int argc, char** argv)
{
    //create 2 processes

    mqd_t PID_s, PID_d, PID_m;

    if ((PID_s = TEMP_FAILURE_RETRY(mq_open("PID_s", O_RDWR | O_NONBLOCK | O_CREAT, 0600, NULL))) == (mqd_t)-1) // queue opened for both reading and writing
        ERR("mq open s"); // here we open the pin queue
    if ((PID_d = TEMP_FAILURE_RETRY(mq_open("PID_d", O_RDWR | O_CREAT, 0600, NULL))) == (mqd_t)-1)
        ERR("mq open d");
    if ((PID_m = TEMP_FAILURE_RETRY(mq_open("PID_m", O_RDWR | O_CREAT, 0600, NULL))) == (mqd_t)-1)
        ERR("mq open m");

}