CSCI 4780 Spring 2021 Project 3
Jisoo Kim, Will McCarthy, Tyler Scalzo


This program allows for different participants (with different IDs
in their configuration files) to connent to the coordinator, and
exchange messages. This supports registering, deregistering, 
disconnecting, and reconnecting. When disconnected, the 
participant will recieve messages sent while disconnected so
long as it does not exceed the time limit set through
the coordinator's configuration file. Said messages will
be stored in each participant's individual log file
described in their respective configuration files.


Compiling:

Run "make all" in the directory with all the files. 

Running:
server: ./coordinator [Configuration_file_name]
client: ./participant [Configuration_file_name]


Configuration file format (each on own line):

participant:
    participant ID
    message log file name
    IP address, and port of the coordinator (seperated by a space)

coordinator:
    port for listening
    persistance time threshold (in seconds)


This project was done in its entirety by Jisoo Kim, Will McCarthy, 
and Tyler Scalzo. We hereby state that we have not received
unauthorized help of any form.