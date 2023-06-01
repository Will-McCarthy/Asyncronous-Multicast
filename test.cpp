// Just testing some stuff outside of the actual programs.

#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <pthread.h>
#include <queue>

using namespace std;

#define BUFFSIZE 1024 // For char buffs.

void* participantThread(void* arg);

struct msgStruct
{
    char message[BUFFSIZE]; // Message
    time_t msgTime; // Time sent.
    int order; // Order received.
};

int main(int argc, char* argv[])
{
    /*
    time_t seconds;

    queue<msgStruct> thread;

    time(&seconds);

    msgStruct mess, mess2, temp;

    strcpy(mess.message, "this is a test");
    mess.msgTime = seconds;
    mess.order = 1;

    sleep(1);

    strcpy(mess2.message, "another test");
    mess2.msgTime = time(NULL);
    mess2.order = 2;

    thread.push(mess);
    thread.push(mess2);

    cout << thread.front().order << endl;
    */

    pthread_t testThread;

    int arg = 5;

    pthread_create(&testThread, 0, participantThread, &arg);

    sleep(1);

    return EXIT_SUCCESS;
}

// Testing.
void* participantThread(void* arg) 
{
    int* temp = (int*)arg;
    int threadID = *temp;

    cout << "thread: " << threadID << endl;

    return NULL;
} // participantThread()