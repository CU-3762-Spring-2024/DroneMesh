#ifndef DRONE_H
#define DRONE_H
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "drone.h"

// Information associated with each drone
struct Drone {
    char drone_ip[20];
    int port_number;
    int in_path;
    int seq_num;
};

// Used to store key-value pairs
struct Pairs {
    // To store an array of strings in C, we must use a 2-D array that stores 25 strings up
    // to 50 characters long
    char keys[25][50];
    char values[25][50];
};

// Used to keep track of drone location
struct Coordinates {
    int x;
    int y;
};

struct Args {
    // Needed to save and print key-value pairs
    struct Pairs pairs;
    // Used to keep track of number of key-value pairs in struct
    int num_of_pairs;
    // Used as a flag to indicate whether message sent from client
    // was meant for me
    int is_my_message;
    int is_in_range;
    int is_alive;
    int is_move_cmd;
    int port_number; 
    // Structure that contains coordinates of my drone
    // based on my_location
    struct Coordinates my_coordinates;
    // Dimensions of the grid 
    int num_rows;
    int num_cols;
    int my_location;
};

// Opens a file and checks for errors
FILE* open_file(char file_name[]);

/* Reads the text file and extracts IP address, port and location for every drone
 * in the network. Each line represents a different drone and this information is saved
 * in a drone struct. Each drone struct is then saved in the array of drones with its
 * associated information */
void read_file(char file_name[], struct Drone drones[], int* num_drones, struct Args *args);

/* Sends the message to a specific drone in the network. Will return rc
 * for error checking purposes */
int send_message(int sock_desc, struct sockaddr_in drone_addr, char msg[]);

/* Receives the message from another drone and calls parse_text to process the message. This function
 * will then call process_msg_type() which will take the appropriate action based on the type of message */
void receive_message(int sock_desc, struct Drone drones[], int* num_drones, struct Args *args);

/* Loads contents of message sent from drone into file and passes the file to lex for analysis. The
 * lex scanner uses regular expressions to match key-value pairs sent from the drone. The regular expressions are defined
 * in the parse.l file. The first regex recognizes key-value pairs that are appended together by matching up to the second
 * semicolon (key:value:) and then taking no action. The remaining value from the second pair wouldn't be a valid pair when
 * lex processes the next lexeme, so this would be ignored. The remaining regex's are for matching the required fields in the
 * message. Finally, everything else is ignored including spaces, newline characters, and tabs */
void parse_text(char msg[], struct Args *args);

/* Separates the key from the value using a colon as the delimiter. The keys/values are then saved
 * in a Pairs struct */
void format_pair(char* key_val_pair, struct Args *args);

// Prints all the key value pairs in the Pairs struct
void print_pairs(struct Args *args);

/* Ensures that the port number is in fact a number and doesn't
 * contain any characters. If the port number isn't valid, the program will exit */
void is_port_a_num(char port_num[]);

// Ensures the port number is within the appropriate range
void check_port_range(int port_num);

// Use the port number and IP address to configure the drone_address structure
void configure_address_struct(struct sockaddr_in *drone_addr, int port_num, char ip_addr[]);

/* Takes the message sent by a drone and appends the fromPort, toPort, TTL, flag,
 * version, fromLocation, myLocation, send-path, time, seqNumber */
void create_reg_message(char msg[], char* line, char port[], struct Args *args, struct Drone drones[], int* num_drones);

/* Calculates x and y coordinates based on drone location and saves this in
 * a struct before returning it to the caller */
struct Coordinates calculate_coordinates(int location, struct Args *args);

/* Listens for input from stdin or from the socket descriptor. If input comes from stdin, a message
 * needs to be send to each drone in the network. If input comes from the socket descriptor, this means
 * the drone is receiving input and the message needs to be processed */
void listen_for_input(int sock_desc, struct Drone drones[], int* num_drones, struct Args *args);

/* Modifies the fields of the message received from the network of drones if the TTL field is
 * greater than 0, the sending drone is in range, and the message is destined for another drone.
 * In this case the TTL field is decremented, and the fromLocation is replaced with the location of
 * my drone */
void modify_fields(char message[], struct Args *args);

/* Returns whether the message received has a msg field (1 if it does) */
int has_message(struct Args *args);

/* When receiving a message from the drone. Check to see if the sequence number in the message
 * sent matches the expected sequence number*/
void create_ack_message(char message[], struct Args *args);

/* Returns whether the message is an ack (based on type field) */
int is_an_ack(struct Args *args);

/* Send to all drones in the network if they aren't already in the send-path */
void send_to_drones(char message[], int sock_desc, struct Drone drones[], int* num_drones, struct Args *args);

/* Receive an ACK message. Sequence number for particular drone gets incremented if the sequence
 * number in the ACK message matches the sequence number in the message originally sent to the drone
 * sending the ACK */
void receive_ack(struct Drone drones[], int* num_drones, struct Args *args);

/* Checks to see if the sequence number in the message received matches the sequence number
 * we expect. If this is the case the message will get printed and the function will return 1. 
 * Otherwise the message will not get printed and the function will return 0. */
int is_expected_message(struct Drone drones[], int* num_drones, struct Args *args);

/* Prints the input message */
void input_msg();

/* Add all the appropriate fields to the message before sending to drone */
void create_message(int sock_desc, struct Drone drones[], int* num_drones, struct Args *args);

/* Will take the appropriate action based on the type of message recieved. There are 4 different cases
 * (see function definition) */
void process_msg_type(char message[], int sock_desc, struct Drone drones[], int* num_drones, struct Args *args);

#endif

