* Bryce Baker
* Student ID: 110587755
* Class: CSCI 3762
* Lab #7
* Due Date: April 11, 2024

# Read Me

### Description of the program:

This lab implements a drone that can send and receive messages and move commands. The program will first read from a config file to get the necessary information for each drone in the network (IP address, port number and location). Then, the program will use a select statement to listen for incoming/outgoing messages. If a regular message is typed into the console, this message is combined with the necessary information (version, to port, from port, ttl, flag, location, sequence number, time, send-path) and sent to each drone in the network (Note: The program will prompt the user to enter in the port number of the drone the message is destined for). Otherwise, if a move command is entered, the drone will send an integer (in addition to the other fields) which will be used to indicate the location for the destination drone to move to. If a message is being received from the network, the drone will take the message received and store it into a buffer. The message will then be loaded into a file for analysis by the lex scanner which will use the regular expressions defined in parse.l to extract valid key-value pairs. For each key-value pair, the scanner will call format_pair() which will separate the key-value pair on the : delimiter and save these values in a key array and value array respectively. Note: The drone will only print messages that are destined for it. The messages that are sent from the network have a to_port field and this must match the port number this drone is running on. In addition the program will check to ensure the sending drone is within range using the euclidean distance formula based on the drone coordinates. If the sending drone is within range and the TTL field is greater than 0, the message will be printed to the console; otherwise, the message will be ignored. If the drone receives a message that is destined for another drone, the sending drone is in range, and the TTL field is greater than 0, the drone will decrement the TTL field and replace the fromLocation value with its own location; the message will then be forwarded to every drone in the network that isn't already in the send-path (to reduce network traffic). If a regular message is received an ACK must be sent to the sending drone. The sending drone will wait for the ACK and ensure the sequence number in the ACK matches the sequence number sent in the original message; otherwise, the message won't be printed. If the ACK does match, the message is printed and the sequence number for that particular drone is incremeneted for the next message (all sequence numbers for each drone is initialized to 1 at the beginning of the program). Finally, if the drone receives a move command, the drone will update its location and associated coordinates and print a message confirming the new location. No ACK message is necessary for a move command and it's assumed that this command can reach any drone in the network directly, without having to propogate through intermediary drones. 


### Source files:

* main.c -- Driver program
* drone.c -- Implements drone functions
* drone.h -- Header file for drone
* parse.l -- Program defines regular expressions and associated actions
 
### Circumstances of program:

* The programs run successfully.
* The program was compiled and run/tested on CSEgrid.

### How to build and run the program:

1. Type 'make' at the command prompt
2. Alternatively,
   * To generate lex.yy.c type 'flex parse.l' at the command prompt
   * To compile drone into executable type 'gcc main.c drone.c lex.yy.c -o drone -lm'
   * To run drone type './drone <port number>'
	
