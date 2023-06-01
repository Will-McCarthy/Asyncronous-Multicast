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

using namespace std;

#define BUFFSIZE 1024 // For char buffs.

int partID, srvPort;
char* logFileName, * srvHost;
char input[BUFFSIZE], message[BUFFSIZE]; // Input, and message buffs.
char* com; // For the command.
bool quit = false; // For ending program.
// [0] = status, [1] = port.
// Status: -1 = non-existant, 0 = dormant, 1 = active.
int threadStats[2] = {-1, -1};

void config(char* fileName); // Parse config.
void getInput(); // Input from the user.
int initConnection(int port); // Creating server connection.
void* bgThread(void* arg); // Thread for receiving messages.
void parseCommand(int srvFD); // Parsing commands.

int main(int argc, char* argv[])
{
    int sockfd;

    if (argc != 2) // Checking if arguments are supplied.
    {
        cout << "Usage: ./participant [config_file_path]" << endl;
        return EXIT_FAILURE;
    }

    config(argv[1]); // Getting info from config file.

    // Printing the configuration.
    cout << "id: " << partID << endl;
    cout << "port: " << srvPort << endl;
    cout << "log: " <<  logFileName << endl;
    cout << "host: " << srvHost << endl << endl;

    while (!quit) // Execute until quit command.
    {
        getInput(); // Getting input from user.

        if ((strcmp(com, "quit") == 0))
        {
            quit = true;
        }
        else
        {
            sockfd = initConnection(srvPort); // Connecting to coordinator.

            parseCommand(sockfd); // Going through the command.

            close(sockfd);
        }
    }

    return EXIT_SUCCESS;
} // main()

void config(char* fileName) // Getting data from the configuration file.
{
    ifstream conFile(fileName);
    string line;
    getline(conFile, line);

    // Participant ID.
    partID = stoi(line);

    // Log file path.
    getline(conFile, line);
    const char* tempLine = line.c_str();
    logFileName = strtok(strdup(tempLine), " ");

    // Service hostname/IP, and port.
    getline(conFile, line);
    tempLine = line.c_str();
    char* anotherTemp = strdup(tempLine);
    srvHost = strtok(anotherTemp, " ");
    srvPort = atoi(strtok(NULL, " "));

    return;
} // config()

void getInput() // Getting input from the user.
{
    do
    {
        cout << "participant> "; // Prompt.
        memset(input, 0, BUFFSIZE); // Resetting the buffer.
        cin.getline(input, BUFFSIZE);
    } while ((strcmp(input, "\0") == 0)); // Retry if just enter.

    memset(message, 0, BUFFSIZE);
    com = strtok(input, " ");

    // Adding rest of input to the message buffer.
    char* temp = strtok(NULL, " ");

    while (temp != NULL) // Getting the rest of the input.
    {
        strcat(message, temp);

        if ((temp = strtok(NULL, " ")) != NULL)
            strcat(message, " ");
    }

    return;
} // getInput()

int initConnection(int port) // Creating server connection. Return fd.
{
    // Conerting the int to a char*.
    char portChar[10];
    sprintf(portChar, "%d", port);

    int sockfd = 0, check = 0;
    struct addrinfo cltAddr, *conInfo;

    // Setting up the host's IP information.
    memset(&cltAddr, 0, sizeof cltAddr);
    cltAddr.ai_family = AF_INET;
    cltAddr.ai_socktype = SOCK_STREAM;
    cltAddr.ai_flags = AI_PASSIVE;

    // Getting connection information.
    getaddrinfo(srvHost, portChar, &cltAddr, &conInfo);

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Getting socket fd.
    if (sockfd < 0)
    {
            cout << "Error creating Socket." << endl;
            exit(EXIT_FAILURE);
    }

    // Connecting to server.
    check = connect(sockfd, conInfo->ai_addr, conInfo->ai_addrlen);
    if (check < 0)
    {
            cout << "Error connecting to server." << endl;
            exit(EXIT_FAILURE);
    }

    return sockfd;
} // initConnection()

void* bgThread(void* arg) // Writes received messages to log file.
{
    FILE* fp;
    fp = fopen(logFileName,"a"); //Open logFile to append any new messages                                                                 
    int prevState = 1; //User is online in the beginning                                                                                   
    char msgBuf[BUFFSIZE]; //Store messages for participants offline                                                                       
    int n=0;
    sleep(0.5);
    int sockfd = initConnection(threadStats[1]); //Create a connection to server to receive messages
    int nfds = sockfd+1; //nfds = highest_numbered_file descriptor plus 1                                                                  

    fd_set sread;
    //FD_SET(sockfd,&sread);

    struct timeval tv;
    tv.tv_sec=5; //Assume timeout is 5 sec                                                                                                 
    tv.tv_usec=0;

    if(fp==NULL){
        cout << "File not opened" << endl;
        exit(EXIT_FAILURE);
    }
    while(1){
        sleep(0.25);
        if(threadStats[0] == -1){ //User deregisters from server                                                                                                                                                                                
        close(sockfd);
        fclose(fp);
        //pthread_exit(NULL);
        return NULL;
        break;
        }
        if(prevState==threadStats[0]){ //User does not change state                                                                          
        if(prevState==1 && threadStats[0]==1){//User is currently online 
            FD_ZERO(&sread);
            FD_SET(sockfd,&sread);                                                   
            if(select((sockfd + 1),&sread,NULL,NULL,&tv)>0){ //Check whether there is message to receive                                             
            if((n=recv(sockfd, msgBuf, BUFFSIZE, 0))>0){ //Receive message only when available                                             
                //cout << "recv" << endl;
                //msgBuf[n] = '\n';
                fputs(msgBuf, fp); //Append message to logFile
                fputs("\n", fp);                                                                            
            }
            }
        }
        else{ //User is currently offline                                                                                                  
            continue;
        }
        }else{ //User changes state                                                                                                          
        if(threadStats[0]==0){ //User changes from online to offline                                                                       
            prevState=0;
            FD_ZERO(&sread); //Clear fd_set                                                                                                  
            close(sockfd); //Close sockfd when user goes offline                                                                             
        }else{ //User changes from offline to online                                                                                       
            prevState=1;
            sleep(0.5);
            sockfd = initConnection(threadStats[1]); //Create a new connection when user is back online                                      
            FD_SET(sockfd,&sread); //Reset FD_SET                                                                                            
        }
        }
    } //while                                                                                                                              
    fclose(fp);

    return NULL;
} // bgThread()

void parseCommand(int sockfd) // Handling commands. srvFD is for connection.
{
    int check = 0; // For error checking.

    // Sending command.
    check = send(sockfd, com, strlen(com), 0);
    if (check < 0)
    {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
    }

    sleep(0.02); // Helps with sync issues.

    if ((strcmp(com, "register") == 0))
    {
        cout << "In register." << endl;

        char partChar[10];
        sprintf(partChar, "%d", partID);

        // Sending ID.
        check = send(sockfd, partChar, strlen(partChar), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        sleep(0.2); // Small pause between sends.

        // Sending port number.
        check = send(sockfd, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        if (threadStats[0] == -1) // Create thread.
        {
            // Updating thread status.
            threadStats[0] = 1;
            threadStats[1] = atoi(message);

            // Still need to create thread.
            pthread_t msgThread;
            pthread_create(&msgThread, NULL, bgThread, NULL);
        }
        else 
            cout << "Already registered." << endl;
        // Don't do anything if thread alredy created.
    }
    else if ((strcmp(com, "deregister") == 0))
    {
        cout << "In deregister." << endl;

        char partChar[10];
        sprintf(partChar, "%d", partID);

        // Sending ID.
        check = send(sockfd, partChar, strlen(partChar), 0);
        if (check < 0)
        {
                cout << "Error sending data." << endl;
                exit(EXIT_FAILURE);
        }

        sleep(0.02); // Helps with sync issues.

        if (threadStats[0] != -1)
        {
            // Updating thread status.
            threadStats[0] = -1;
            threadStats[1] = -1;

            cout << "Deregistered." << endl;
        }
        else // Can't deregister if not registered.
            cout << "Cannot deregister, not registered." << endl;
    }
    else if ((strcmp(com, "disconnect") == 0))
    {
        cout << "In disconnect." << endl;

        char partChar[10];
        sprintf(partChar, "%d", partID);

        // Sending ID.
        check = send(sockfd, partChar, strlen(partChar), 0);
        if (check < 0)
        {
                cout << "Error sending data." << endl;
                exit(EXIT_FAILURE);
        }

        sleep(0.02); // Helps with sync issues.

        if (threadStats[0] != -1)
        {
            // Updating thread status.
            threadStats[0] = 0;

            cout << "Disconnected." << endl;
        }
        else 
            cout << "Cannot disconnect, not registered." << endl;
    }
    else if ((strcmp(com, "reconnect") == 0))
    {
        cout << "In reconnect." << endl;

        char partChar[10];
        sprintf(partChar, "%d", partID);

        // Sending ID.
        check = send(sockfd, partChar, strlen(partChar), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        sleep(0.2); // Small pause between sends.

        // Sending port number.
        check = send(sockfd, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        if (threadStats[0] == 0) // Reactivate.
        {
            // Updating thread status.
            threadStats[0] = 1;
            threadStats[1] = atoi(message);
        }
    }
    else if ((strcmp(com, "msend") == 0))
    {
        cout << "In msend." << endl;

        // Sending message.
        check = send(sockfd, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        sleep(0.02); // Helps with sync issues.

        // Waiting for acknowledgement before continuing.
        check = recv(sockfd, message, BUFFSIZE, 0);
        if (check < 0)
        {
                cout << "Error receiving data." << endl;
                exit(EXIT_FAILURE);
        }
    }
    else
    {
        cout << "Command invalid." << endl;
    }
} // parseCommand()