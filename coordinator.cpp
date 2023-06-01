// CSCI 4780 Spring 2021 Project 3
// Jisoo Kim, Will McCarthy, Tyler Scalzo

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
#include <mutex>

using namespace std;

#define BACKLOG 5 // Backlog for listen.
#define BUFFSIZE 1024 // For char buffs.

// Struct for messages.
struct msgStruct
{
    char message[BUFFSIZE]; // Message
    time_t msgTime; // Time received.
    int orderNum; // Order received.
};

int listPort, persistTime; // Port for connections, and message lifetime.
const int maxParticipants = 5; // Max number of participants.
// For holding participant information. [0] = status. [1] = port, [2] = partID.
// Status: -1 = Unused, 0 = offline, 1 = active.
int partInfo[maxParticipants][3];
queue<msgStruct> msgQueue[maxParticipants]; //holds the messages for each client
int msgOrder = 0; // Message order number.

void config(char* fileName); // Parse config.
int initConnection(int port); // Creating server connection.
void initPartList(); // Initiates the participant list array.
int participantManagement(int rule, int id, int port); // Participant handling.
void* participantThread(void* arg); // Thread for participants.
void parseCommand(int cltFD); // Handling commands. cltFD is for connection.
void printParticipants(); // For testing only. Prints participant array.
mutex mutexQ; //for maintaining queue consistency

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "Usage: ./coordinator [config_file_path]" << endl;
        return EXIT_FAILURE;
    }

    config(argv[1]); // Getting info from config file.

    // Printing the configuration.
    cout << "port: " << listPort << endl;
    cout << "time: " << persistTime << endl;

    initPartList(); // Initializing participant list.

    // Creating connection for main thread, and related things.
    int sockFD = initConnection(listPort), accFD;
    socklen_t cltAddrSize = 0;
    struct sockaddr cltAddr;

    while (true) // Keep executing.
    {
        accFD = accept(sockFD, &cltAddr, &cltAddrSize);
        if (accFD < 0)
        {
            cout << "Error accepting." << endl;
            exit(EXIT_FAILURE);
        }

        parseCommand(accFD);

        close(accFD);

        printParticipants(); // Testing thing.
    }

    return EXIT_SUCCESS;
} // main()

void config(char* fileName) // Getting data from the configuration file.
{
    ifstream conFile(fileName);
    string line;

    // Port for listening..
    getline(conFile, line);
    listPort = stoi(line);

    // Time threshold.
    getline(conFile, line);
    persistTime = stoi(line);

    return;
} // config()

int initConnection(int port) // Create socket for listening.
{
    // Conerting the int to a char*.
    char portChar[10];
    sprintf(portChar, "%d", port);

    struct addrinfo srvAddr, *conInfo;

    //create the socket
    int sockFD = socket(AF_INET, SOCK_STREAM, 0); // Getting socket fd.
    if (sockFD < 0)
    {
            cout << "Error creating Socket." << endl;
            exit(EXIT_FAILURE);
    }

    //Getting stuff setup for socket binding
    memset(&srvAddr, 0, sizeof srvAddr);
    srvAddr.ai_family = AF_INET;
    srvAddr.ai_socktype = SOCK_STREAM;
    srvAddr.ai_flags = AI_PASSIVE;

    // Getting connection information.
    getaddrinfo(NULL, portChar, &srvAddr, &conInfo);
    // Binding to the port.
    int check = bind(sockFD, conInfo->ai_addr, conInfo->ai_addrlen);
    if (check < 0)
    {
            cout << "Error binding port." << endl;
            exit(EXIT_FAILURE);
    }

    // Listening for connections.
    check = listen(sockFD, BACKLOG);
    if (check < 0)
    {
            cout << "Error listening." << endl;
            exit(EXIT_FAILURE);
    }

    return sockFD;
} // initConnection()

void initPartList() // Initializes list with default values.
{
    for (int i = 0; i < maxParticipants; i++)
    {
        partInfo[i][0] = -1; // No participant.
        partInfo[i][1] = -1; // Not yet defined.
        partInfo[i][2] = -1; // Not yet defined.
    }
} // initPartList()

// Handles registering, deregistering, and disconnecting of participands.
// rule: -1 = deregistering, 0 = disconnect, 1 = register/reconnect.
int participantManagement(int rule, int id, int port)
{
    if (rule == 1) // Registering.
    {
        int empty = -1; // For finding a spot to put participant.
        int i = 0;
        bool done = false;

        while (!done)
        {
            if (partInfo[i][0] == -1) // Finding unused spot.
            {
                if (i > empty)
                    empty = i;
            }

            if (partInfo[i][2] == id) // If already in participant list.
            {
                if (partInfo[i][0] == 0) // Reconnect after disconnet.
                {
                    partInfo[i][1] = port;
                    partInfo[i][0] = 1;

                    return (i + maxParticipants);
                }
                else // Already registered, and active.
                    return -2;
            }

            i++;

            if (i == maxParticipants) // Finished parsing list.
                done = true;
        }

        if (empty != -1) // Adding new participant to list.
        {
            partInfo[empty][1] = port;
            partInfo[empty][2] = id;
            partInfo[empty][0] = 1;

            return empty;
        }
        else // List full.
            return -1;
    }
    else if (rule == 0) // Disconnecting.
    {
        for (int i = 0; i < maxParticipants; i++)
        {
            if (partInfo[i][2] == id) // Mark as disconnected.
            {
                partInfo[i][0] = 0;
                return (i + maxParticipants);
            }
        }

        return -1; // Nothing disconnected.
    }
    else // Deregistering.
    {
        for (int i = 0; i < maxParticipants; i++)
        {
            if (partInfo[i][2] == id) // Deregister.
            {
                // Reset values
                partInfo[i][1] = -1;
                partInfo[i][2] = -1;
                partInfo[i][0] = -1;

                return i;
            }
        }

        return -1; // Nothing deregistered.
    }
} // participantManagement()

void* participantThread(void* arg) // Handles sending things to participants.
{
    // An int will be passes, so making it an actual int.
    int* temp = (int*)arg;
    int threadID = *temp; // threadID is the array index for Thread's info.
    // Actual work.

    ////Pseudocode
    //make a connection using the correct port
    // check status of connection
    //end on -1, wait to recconect on 0, send messages on 1
    //in send messages, check if there is a message in the queue
    //then pop message
    //check if message is out of date
    //send message

    //// For holding participant information. [0] = status. [1] = port, [2] = partID.
    // Status: -1 = Unused, 0 = offline, 1 = active.
    //int partInfo[maxParticipants][3];

    int sockfd = initConnection(partInfo[threadID][1]);
    socklen_t cltAddrSize = 0;
    struct sockaddr cltAddr;

    int accFD = accept(sockfd, &cltAddr, &cltAddrSize);
    if (accFD < 0)
    {
        cout << "Error accepting." << endl;
        exit(EXIT_FAILURE);
    }

    cout << "thread created" << endl;

    while(true){
      if (partInfo[threadID][0] == -1) return NULL; //connection Disconnected
      else if (partInfo[threadID][0] == 0){//connection temporarily offline
        close(accFD); // Close connection.
        close(sockfd); //close the old sockfd
        while(partInfo[threadID][0] == 0){sleep(0.25);}//wait until active again
        if (partInfo[threadID][0] == 1)//reconnect if it goes active
        { 
          sockfd = initConnection(partInfo[threadID][1]);
          accFD = accept(sockfd, &cltAddr, &cltAddrSize);
            if (accFD < 0)
            {
                cout << "Error accepting." << endl;
                exit(EXIT_FAILURE);
            }
        }
      }
      else if (partInfo[threadID][0] == 1){ //connection active
        if(msgQueue[threadID].size() > 0){ //if there are new messages to be recieved
          //read/pop message from the queue
          mutexQ.lock();
          msgStruct msg = msgQueue[threadID].front(); //get the message
          msgQueue[threadID].pop(); //delete it from queue
          mutexQ.unlock();

          if((time(NULL) - msg.msgTime) < persistTime){ //only send messages if they arent old
            int check = send(accFD, msg.message, BUFFSIZE, 0); //send message to client
            cout << "thread: " << threadID << " sent message." << endl;
            if (check < 0)
            {
              cout << "Error sending data." << endl;
              exit(EXIT_FAILURE);
            }//if check
          }//if time
        }//if msgQueue
      }//if partInfo

      sleep(0.25);
    }// while
    return NULL;
} // participantThread()

void parseCommand(int cltFD) // Handling commands. srvFD is for connection.
{
    int check = 0; // For error checking.
    char netBuff[BUFFSIZE], message[BUFFSIZE]; // Net, and message buffs.
    char com[20]; // For the command.

    // Resetting buffers.
    memset(netBuff, 0, BUFFSIZE);
    memset(message, 0, BUFFSIZE);
    memset(com, 0, 20);

    // Recieving command.
    check = recv(cltFD, netBuff, BUFFSIZE, 0);
    if (check < 0)
    {
            cout << "Error receiving data." << endl;
            exit(EXIT_FAILURE);
    }

    strcpy(com, netBuff); // Getting command.
    cout << "command: " << com << endl; // For testing.
    memset(netBuff, 0, BUFFSIZE);

    // Recieving message.
    check = recv(cltFD, netBuff, BUFFSIZE, 0);
    if (check < 0)
    {
            cout << "Error receiving data." << endl;
            exit(EXIT_FAILURE);
    }

    strcpy(message, netBuff); // Getting rest of message.

    if ((strcmp(com, "register") == 0))
    {
        //cout << "In register." << endl;

        int partID = atoi(message);

        memset(netBuff, 0, BUFFSIZE);

        // Recieving portNumber.
        check = recv(cltFD, netBuff, BUFFSIZE, 0);
        if (check < 0)
        {
                cout << "Error receiving data." << endl;
                exit(EXIT_FAILURE);
        }

        int port = atoi(netBuff);

        check = participantManagement(1, partID, port);

        if (check >= 0) // If normal register.
        {
            cout << "Participant ID: " << partID << " registerd." << endl;

            /*
            // Still need to create the thread.

            // Check has the spot in the participant array that holds the
            // thread's information. Passing that so the thread can get
            // it's actually needed information.
            pthread_create(&testThread, 0, participantThread, &check);

            */
            pthread_t thread_ID; //do we need to store this or do somthing with this?!?!
            pthread_create(&thread_ID, NULL, participantThread, &check);
        }
        else if (check == -1)
        {
            cout << "Could not register Participant: " << partID << endl;
        }
        else
        {
            cout << "Already registered Participant: " << partID << endl;
        }
    }
    else if ((strcmp(com, "deregister") == 0))
    {
        //cout << "In deregister." << endl;

        int partID = atoi(message);

        check = participantManagement(-1, partID, -1); // Don't need port.

        if (check == -1)
            cout << "Cannot deregister, not registered." << endl;
        else
            cout << "Deregistered participant: " << partID << endl;
    }
    else if ((strcmp(com, "disconnect") == 0))
    {
        //cout << "In disconnect." << endl;

        int partID = atoi(message);

        check = participantManagement(0, partID, -1); // Don't need port.

        if (check == -1)
            cout << "Cannot disconnect, not registered." << endl;
        else
            cout << "Disconnected participant: " << partID << endl;
    }
    else if ((strcmp(com, "reconnect") == 0))
    {
        //cout << "In reconnect." << endl;

        int partID = atoi(message);

        memset(netBuff, 0, BUFFSIZE);

        // Recieving portNumber.
        check = recv(cltFD, netBuff, BUFFSIZE, 0);
        if (check < 0)
        {
                cout << "Error receiving data." << endl;
                exit(EXIT_FAILURE);
        }

        int port = atoi(netBuff);

        check = participantManagement(1, partID, port);

        if (check > 0)
            cout << "Reconnected participant: " << partID << endl;
        else
            cout << "Could not reconnect." << endl;
    }
    else if ((strcmp(com, "msend") == 0))
    {
        //cout << "In msend." << endl;

        // msgTime is the time the message was received.
        time_t msgTime;
        time(&msgTime);

        msgOrder++; // Order number of the message.

        // These two cout statements are for testing. Can be deleted.
        cout << "msgTime: " << msgTime << " msgOrder: " << msgOrder << endl;
        // message is the message to put in the participant message queues.
        cout << message << endl;

        // Code for putting into struct, and queues here.

        struct msgStruct msg; //create a msgStruct to hold all the data
        strcpy(msg.message, message);
        msg.msgTime = msgTime;
        msg.orderNum = msgOrder;

        for(int i = 0; i < maxParticipants; i++){//for each participant
          if(partInfo[i][0] != -1){ //only push msg if that participant is active
            mutexQ.lock();
            msgQueue[i].push(msg);//put the message in their queue
            mutexQ.unlock();
          }//if partinfo
        }//for

        // Sending an acknowledgement to client.
        check = send(cltFD, "ack", 3, 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        cout << "Invalid command received." << endl;
    }
} // parseCommand()

void printParticipants() // Prints participant info. Mostly for testing.
{
    cout << endl << "status" << "\tport" << "\tpartID" << endl;

    for (int i = 0; i < maxParticipants; i++)
    {
        cout << "part " << i << ": " << partInfo[i][0] << "\t"
            << partInfo[i][1] << "\t" << partInfo[i][2] << endl;
    }

    cout << endl;
} // printParticipants()