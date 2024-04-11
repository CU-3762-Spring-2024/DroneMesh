#ifndef DRONE_H
#define DRONE_H
#include <netinet/in.h>

struct Drone {
    char drone_ip[20];
    int port_number;
    int id;
    int seq_num;
};

// Used to store key-value pairs
struct Pairs {
    // To store an array of strings in C, we must use a 2-D array that stores 25 strings up
    // to 50 characters long
    char keys[25][50];
    char values[25][50];
};

struct Coordinates {
    int x;
    int y;
};

// Opens a file and checks for errors
FILE* open_file(char file_name[]);

/* Reads the text file and extracts IP address and port for every drone
 * in the network. Each line represents a different drone and this information is saved
 * in a drone struct. Each drone struct is then saved in the array of drones with its
 * associated information */
void read_file(char file_name[], struct Drone drones[], int* num_drones, int* my_location);

/* Sends the message to each drone in the network. Will return rc
 * for error checking purposes */
int send_message(int sock_desc, struct sockaddr_in drone_addr, char msg[]);

/* Receives the message from another drone and prints what was received
 * if the message was destined for this drone. If the message is destined for another drone
 * , the TTL field is greater than 0 and the sending drone is in range, the message will be
 * forwarded to every drone in the network with an updated TTL and fromLocation */
void receive_message(int sock_desc, struct Drone drones[], int* num_drones, int* my_location);

/* Loads contents of message sent from drone into file and passes the file to lex for analysis. The
 * lex scanner uses regular expressions to match key-value pairs sent from the drone. The regular expressions are defined
 * in the parse.l file. The first regex recognizes key-value pairs that are appended together by matching up to the second
 * semicolon (key:value:) and then taking no action. The remaining value from the second pair wouldn't be a valid pair when
 * lex processes the next lexeme, so this would be ignored. The remaining regex's are for matching the required fields in the
 * message. Finally, everything else is ignored including spaces, newline characters, and tabs */
void parse_text(char msg[], char file_name[]);

/* Separates the key from the value using a colon as the delimiter. The keys/values are then saved
 * in a Pairs struct */
void format_pair(char* key_val_pair);

// Prints all the key value pairs in the Pairs struct
void print_pairs(int my_location);

/* Ensures that the port number is in fact a number and doesn't
 * contain any characters. If the port number isn't valid, the program will exit */
void is_port_a_num(char port_num[]);

// Ensures the port number is within the appropriate range
void check_port_range(int port_num);

// Use the port number and IP address to configure the drone_address structure
void configure_address_struct(struct sockaddr_in *drone_addr, int port_num, char ip_addr[]);

/* Takes the message sent by a drone and appends the fromPort, toPort, TTL, flag,
 * version, fromLocation and myLocation */
void create_message(char msg[], char* line, char port[], int* my_location, struct Drone drones[], int* num_drones);

/* Calculates x and y coordinates based on drone location and saves this in
 * a struct before returning it to the caller */
struct Coordinates calculate_coordinates(int location);

/* Listens for input from stdin or from the socket descriptor. If input comes from stdin, a message
 * needs to be send to each drone in the network. If input comes from the socket descriptor, this means
 * the drone is receiving input and the message needs to be processed */
void listen_for_input(int sock_desc, struct Drone drones[], int* num_drones, int* my_location);

/* Modifies the fields of the message received from the network of drones if the TTL field is
 * greater than 0, the sending drone is in range, and the message is destined for another drone.
 * In this case the TTL field is decremented, and the fromLocation is replaced with the location of
 * my drone */
void modify_fields(char message[], int* my_location);

/*
*/
int has_message();

/*
*/
void create_ack_message(char message[], int* my_location);

/*
*/
int is_an_ack();

/*
*/
void send_to_drones(char message[], int sock_desc, struct Drone drones[], int* num_drones);

/*
*/
void receive_ack(struct Drone drones[], int* num_drones, int* my_location);

/*
*/
void is_expected_message(struct Drone drones[], int* num_drones, int* my_location);

#endif

