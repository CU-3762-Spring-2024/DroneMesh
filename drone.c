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
#define STDIN 0

/* NOTE: These global variables are needed for the lex.yy.c file.
 * Will restructure and remove global variables for the next lab */

// Needed to save and print key-value pairs
struct Pairs pairs;
// Used to keep track of number of key-value pairs in struct
int num_of_pairs = 0;
// Used as a flag to indicate whether message sent from client
// was meant for me
int is_my_message;
int is_in_range;
int is_alive;
int port_number = 0;
// Structure that contains coordinates of my drone
// based on my_location
struct Coordinates my_coordinates;
// Dimensions of the grid 
int num_rows;
int num_cols;

// Main driver
int main(int argc, char *argv[])
{
    int sd; /* the socket descriptor */
    struct sockaddr_in drone_address; /* my address */
    int rc; // always need to check return codes!
    int num_drones = 0;
    // Array of drones, can hold up to 50
    struct Drone drones[50];
    // Variable that represents my location in the grid
    int my_location;

    // Check to see if the right number of parameters was entered
    if (argc < 2)
    {
        printf ("usage is drone <port number>\n");
        exit(1); /* just leave if wrong number entered */
    }

    // Ask the user to specify the dimensions of the grid
    printf("Enter the number of columns in the grid: ");
    scanf("%d", &num_cols);
    printf("Enter the number of rows in the grid: ");
    scanf("%d", &num_rows);
    // Consume the newline character
    getchar();

    // First create a socket using UDP protocol
    sd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */
    // Print error and exit if invalid socket descriptor
    if(sd < 0)
    {
        perror("Invalid socket descriptor\n");
        exit(1);
    }

    // Check if port num is a num
    is_port_a_num(argv[1]);

    // Convert from string to long
    port_number = strtol(argv[1], NULL, 10);

    // Ensure port num is within correct range
    check_port_range(port_number);

    // Configure the drone_address struct
    configure_address_struct(&drone_address, port_number, NULL);

    char config_file_name[] = "config.file";
    // Read the config file first
    read_file(config_file_name, drones, &num_drones, &my_location);

    // The next step is to bind to the address
    rc = bind (sd, (struct sockaddr *)&drone_address,
               sizeof(struct sockaddr ));
    // Check if there was an error binding to socket
    if (rc < 0)
    {
        perror("bind");
        exit (1);
    }

    // Drone will listen for message from network or outgoing
    // message from stdin
    listen_for_input(sd, drones, &num_drones, &my_location);
}
// End of main

FILE* open_file(char file_name[]) {
    FILE *file;
    // Open the file in read binary mode
    file = fopen(file_name, "r");
    // Check if the file was opened successfully
    if (file == NULL)
    {
        perror("Input file failed to open\n");
        exit(1);
    }
    return file;
}

void read_file(char file_name[], struct Drone drones[], int* num_drones, int* my_location) {
    size_t len = 0; /* size of the line buffer */
    char *line = NULL;
    FILE* stream = open_file(file_name);

    // Extract each line from the file
    while(getline(&line, &len, stream) != -1)
    {
        char *token;
        // IP addr and port are separated by a space
        const char delimiter[] = " ";
        // Extract token delimited by the space
        token = strtok(line, delimiter);
        // Copy the ip address into the structure
        strcpy(drones[*num_drones].drone_ip, token);
        // Get the next token (value)
        token = strtok(NULL, delimiter);
        // Check if the port is an actual number
        is_port_a_num(token);
        // Convert the string to long
        drones[*num_drones].port_number = strtol(token, NULL, 10);
        // Ensure the port number is within correct range
        check_port_range(drones[*num_drones].port_number);
        int drone_port = strtol(token, NULL, 10);
        // Grab the drone location
        token = strtok(NULL, delimiter);
        // Remove the newline character
        token[strlen(token) - 1] = '\0';
        // If it's my port
        if(port_number == drone_port)
        {
            // Record my location
            *my_location = strtol(token, NULL, 10);
            printf("Location is %d\n", *my_location);
            // Calculate my coordinates
            my_coordinates = calculate_coordinates(*my_location);
        }
        drones[*num_drones].id = *num_drones;
        drones[*num_drones].seq_num = 1;
        (*num_drones)++;
    }
    // Ensure the file is closed when finished
    fclose(stream);
}

int send_message(int sock_desc, struct sockaddr_in drone_addr, char msg[])
{
    int rc;
    char buffer_out[512];
    // Clear the buffer
    memset(buffer_out, 0, 512);
    // Copy the message to the buffer
    sprintf(buffer_out, "%s", msg);
    // Send buffer to the server
    rc = sendto(sock_desc, buffer_out, strlen(buffer_out), 0,
                (struct sockaddr *) &drone_addr, sizeof(drone_addr));
    return rc;
}

void is_port_a_num(char port_num[])
{
    // Iterate through each character
    for(int i = 0; i <strlen(port_num); i++)
    {
        // Ensure each character is a digit
        if (!isdigit(port_num[i]))
        {
            printf ("The Portnumber isn't a number!\n");
            exit(1);
        }
    }
}

void check_port_range(int port_num)
{
    // Exit if port number is too big or too small
    if ((port_num > 65535) || (port_num < 0))
    {
        printf ("you entered an invalid socket number\n");
        exit(1);
    }

}

void configure_address_struct(struct sockaddr_in *drone_addr, int port_num, char ip_addr[])
{
    drone_addr->sin_family = AF_INET; /* use AF_INET addresses */
    drone_addr->sin_port = htons(port_num); /* convert port number */
    // If an IP address was passed as an argument (not NULL)
    if(ip_addr)
        drone_addr->sin_addr.s_addr = inet_addr(ip_addr); /* convert IP addr */
    // Otherwise use any IP address
    else
        drone_addr->sin_addr.s_addr = INADDR_ANY;
}

void receive_message(int sock_desc, struct Drone drones[], int* num_drones, int* my_location) {
    int rc;
    struct sockaddr_in from_address;  /* address of sender */
    char buffer_received[512]; // used in recvfrom
    int flags = 0; // used for recvfrom
    socklen_t from_length;
    // Each instance of the server needs its own
    // unique file name (if it's ran on the same machine), so the port number is
    // used to create a unique file name for the drone. This is necessary as the lex scanner needs
    // each line sent from the client to be written to a file for analysis. If each instance
    // of the server uses the same file this will cause incorrect results since they
    // are attempting to read and write to the same file simultaneously
    char file[10];
    sprintf(file, "%d", port_number);
    // Add the .txt extension to the file name
    strcat(file, ".txt");

    // from_length must have an initial value
    from_length = sizeof(struct sockaddr_in);
    // Assume the message isn't for me until I confirm the port
    // number in the message
    is_my_message = 0;
    // Clear the buffer in each iteration
    memset (buffer_received, 0, 512);
    rc = recvfrom(sock_desc, buffer_received, 512, flags,
                  (struct sockaddr *) &from_address, &from_length);
    // Exit if there was an issue with the receive
    if(rc < 0)
    {
        perror("Issue receiving bytes.\n");
        exit(1);
    }
    parse_text(buffer_received, file);
    /* Three cases:
     * 1) If the message is meant for me, is in range and still alive, print the message and send an ACK to every drone
     * 2) If the message for me is an ACK sent from another drone (from a message I sent), print the message but dont resend
     * 3) Otherwise, if the message is meant for another drone, is in range and still alive,
     *    forward the message to every drone in the network (before sending the message, replace
     *    the fromLocation field with my location and decrement the TTL field. */
    if(is_my_message && is_in_range && is_alive)
    {
        // If receiving an ACK message for previously sent message
        if(is_an_ack())
        {
            // Process the ACK message
            receive_ack(drones, num_drones, my_location);
        }
        // Else the message is not an ACK message
        else
        {
            // Process the (non-ACK) message and be sure it's what we expect
            // based on the sequence number for that particular drone
            is_expected_message(drones, num_drones, my_location);
            // If the message sent contains a msg field, 
            // an ACK needs to be sent to the sending drone
            if(has_message())
            {
                // Create ACK fields
                create_ack_message(buffer_received, my_location);
                send_to_drones(buffer_received, sock_desc, drones, num_drones);
            }
        }
    }
    // If the message isn't for me but is still in range and alive
    else if(!is_my_message && is_in_range && is_alive)
    {
        // Update the fields in the message with my information
        modify_fields(buffer_received, my_location);
        send_to_drones(buffer_received, sock_desc, drones, num_drones);
    }
    // Clear the struct storing the key-value pairs by
    // setting num_of_pairs to 0. This will ensure the previous
    // data will get overwritten when the next line is received
    // and processed
    num_of_pairs = 0;
}

/* Note: the lex scanner requires input from the console or from a file.
 * We set the variable yyin to point to a file which tells lex we want it
 * to read input from this file rather than the console. Also, with the lex
 * scanner, it wouldn't allow me to open the file in read/write mode so I had
 * to open the file in write mode to write to the file, then open it again in
 * read mode for the scanner itself
 * */
void parse_text(char msg[], char file_name[]) {
    // Extern variables defined in lex.yy.c
    extern int yylex();
    extern FILE *yyin;
    // Open the file in write mode
    yyin = fopen(file_name, "w");
    // Print contents of message to file
    fprintf(yyin, "%s", msg);
    fclose(yyin);

    // Open the file in read mode
    yyin = fopen(file_name, "r");
    // Pass file to the scanner for analysis
    yylex();
    fclose(yyin);
}

void format_pair(char* key_val_pair) {
    char *token;
    const char delimiter[] = ":";

    // Extract token delimited by the colon
    token = strtok(key_val_pair, delimiter);
    // Copy the first token (key) to pairs.keys struct
    strcpy(pairs.keys[num_of_pairs], token);
    // Get the next token (value)
    token = strtok(NULL, delimiter);
    // Copy the token to pairs.values struct
    strcpy(pairs.values[num_of_pairs], token);
    num_of_pairs++;
}

void print_pairs(int my_location) {
    // Print all key-value pairs
    printf("\t\t    ------------------------------------------\n");
    printf("\t\t%20s", "myLocation");
    printf("%20d\n", my_location);
    for(int i = 0; i < num_of_pairs; i++)
    {
        // Print the key
        printf("\t\t%20s", pairs.keys[i]);
        // Print the value
        printf("%20s\n", pairs.values[i]);
    }
    printf("\t\t    ------------------------------------------\n");
}

void create_message(char msg[], char* line, char port[], int* my_location, struct Drone drones[], int* num_drones) {
    char tmp[20];
    
    // Add the message field
    strcat(msg, line);
    // Add the closing quote
    strcat(msg, "\" ");
    
    // Add the toPort field
    strcat(msg, "toPort:");
    // Append the port number
    strcat(msg, port);
    strcat(msg, " ");

    // Add the fromPort field
    strcat(msg, "fromPort:");
    // Convert from_port to string and store in temp
    sprintf(tmp, "%d", port_number);
    strcat(msg, tmp);
    strcat(msg, " ");

    // Add the version field
    strcat(msg, "version:6 ");

    // Add the location field
     strcat(msg, "location:");
    // Convert location to a string and store in tmp
    sprintf(tmp, "%d", *my_location);
    // Add location to message
    strcat(msg, tmp);
    strcat(msg, " ");
    
    // Add the TTL field
    strcat(msg, "TTL:6 ");
    
    // Add the flag field
    strcat(msg, "flags:0 ");

    // Add the send-path field
    strcat(msg, " send-path:");
    // Convert my port to a string and store in tmp
    sprintf(tmp, "%d ", port_number);
    // Add my port number to the message
    strcat(msg, tmp);
    
    // For the sequence number
    int seq;
    // First, lookup what drone we're sending the message
    // to and attach the associated seq num for the drone
    // to the message
    for(int i = 0; i < *num_drones; i++)
        if(drones[i].port_number == atoi(port))
            seq = drones[i].seq_num;
        
    // Add the sequence number field
    strcat(msg, "seqNumber:");
    // Convert sequence number to a string and store in tmp
    sprintf(tmp, "%d", seq);
    // Add the sequence number to the message
    strcat(msg, tmp);
    strcat(msg, " ");

    // Get the current time
    time_t t = time(NULL);
    // Add the time field to the message
    strcat(msg, "time:");
    // Conver the time to a string and store in tmp
    sprintf(tmp, "%ld", t);
    // Add the time to the message
    strcat(msg, tmp);
    strcat(msg, " ");
}


struct Coordinates calculate_coordinates(int location) { 
    int row_num = -1;
    int col_num;
    /* Continuously subtract the number of columns from the grid location. Each time
     * we subract the number of columns, increment the row number. In the last iteration,
     * once location <= num_cols we record the col_num and the loop exits */
    while(location > 0)
    {
        // Index starts from 0 so we subtract 1
        col_num = location - 1;  
        location -= num_cols;  
        row_num++;
    }
    struct Coordinates coordinates;
    // Save the x and y coordinate in the struct
    coordinates.x = col_num;
    coordinates.y = row_num;
    return coordinates;
}

void listen_for_input(int sock_desc, struct Drone drones[], int* num_drones, int* my_location) {
    // For the select
    fd_set socket_fds; // The socket descriptor set
    int max_sd; // Tells the OS how many sockets are set
    struct sockaddr_in to_address; /* my address */
    int rc; // always need to check return codes!

    printf("Type message: \n");
    // Continue forever (until the drone is turned off)
    for(;;)
    {
        FD_ZERO(&socket_fds); // Remove all file descriptors from set
        FD_SET(sock_desc, &socket_fds); // Add the socket descriptor to set
        FD_SET(STDIN, &socket_fds); // Add the stdin file descriptor
        // Record which is associated with larger integer (STDIN or sd)
        if(STDIN > sock_desc)
            max_sd = STDIN;
        else
            max_sd = sock_desc;
        // Block until something arrives
        rc = select(max_sd + 1, &socket_fds, NULL, NULL, NULL);
        // Check for errors
        if(rc < 0)
        {
            perror("Error in select!\n");
            exit(1);
        }
        // If message was received from console
        if(FD_ISSET(STDIN, &socket_fds))
        {
            size_t len = 0; /* size of the line buffer */
            char *line = NULL;
            // Read the line from console and continue if no errors
            if(getline(&line, &len, stdin) != -1)
            {
                // Remove the new line character
                line[strlen(line) - 1] = '\0';
                char msg[512];
                // Prompt the user to enter destination port. This value
                // will be appended to the message send to each drone in the network
                char destination_port[6];
                printf("What port number is this message destined for?\n");
                scanf("%s", destination_port);
                // Consume the newline character (when user presses enter)
                getchar();
                // Send each line to all the drones in the network
                for(int i = 0; i < *num_drones; i++)
                {
                    // Don't send to myself
                    if (drones[i].port_number != port_number)
                    {
                        // Clear the msg buffer in each iteration
                        memset(msg, 0, 512);
                        // Add the beginning part of the message
                        strcat(msg, "msg:\"");
                        configure_address_struct(&to_address, drones[i].port_number, drones[i].drone_ip);
                        // Send the message to the ith drone
                        create_message(msg, line, destination_port, my_location, drones, num_drones);
                        rc = send_message(sock_desc, to_address, msg);
                        // Check for any errors
                        if (rc < 0)
                        {
                            printf("Issue sending message to drone.\n");
                            exit(1);
                        }
                    }
                }
            }
            else
            {
                printf("Error reading from console!\n");
                exit(1);
            }
        }
        // If a message is being sent over the network
        if(FD_ISSET(sock_desc, &socket_fds))
        {
            // Start receiving message from the server
            receive_message(sock_desc, drones, num_drones, my_location);
        }
    }
}

void modify_fields(char message[], int* my_location) {
    memset(message, 0, 512);
    for(int i = 0; i < num_of_pairs; i++)
    {
        char temp[20];
        // Evaluates to true if key matches 'send-path'
        if(!strcmp(pairs.keys[i], "send-path")) 
        {
            // Add port number to send path
            sprintf(temp, ",%d ", port_number);
            strcat(pairs.values[i], temp);
            strcat(message, pairs.keys[i]);
            strcat(message, ":");
            strcat(message, pairs.values[i]);
            continue;
        }
        // Evaluates to true if key matches 'TTL'
        else if(!strcmp(pairs.keys[i], "TTL"))
        {
            int ttl = atoi(pairs.values[i]);
            ttl--;
            sprintf(temp, "%d", ttl);
            strcpy(pairs.values[i], temp);
            strcat(message, pairs.keys[i]);
            strcat(message, ":");
            strcat(message, pairs.values[i]);
            strcat(message, " ");
            continue;
        }
        // Evaluates to true if key matches 'location'
        else if(!strcmp(pairs.keys[i], "location"))
        {
            sprintf(temp, "%d ", *my_location);
            strcpy(pairs.values[i], temp);
            strcat(message, pairs.keys[i]);
            strcat(message, ":");
            strcat(message, pairs.values[i]);
            strcat(message, " ");
            continue;
        }
        else
        {
            // Append the other fields without any changes
            strcat(message, pairs.keys[i]);
            strcat(message, ":");
            strcat(message, pairs.values[i]);
            strcat(message, " ");
        }
    }
}

// Return 0 if false and 1 if true
int has_message() {
    // Iterate through all the pairs
    for(int i = 0; i < num_of_pairs; i++)
        // If the message has a msg field
        if(!strncmp(pairs.keys[i], "msg", 3))
            return 1;
    return 0;
}

void create_ack_message(char message[], int* my_location) {
    // Clear the message
    memset(message, 0, 512);
    // Add the type to message
    strcpy(message, "type:ACK ");
    char temp[10];
    int index_from_port = 0;
    int index_to_port = 0;
    for(int i = 0; i < num_of_pairs; i++)
    {
       // If the key is not 'msg'
       if(strcmp(pairs.keys[i], "msg"))
       {
            // Update the send-path by adding my port
            if(!strcmp(pairs.keys[i], "send-path"))
            {
                char temp[18];
                sprintf(temp, "%d ", port_number);
                strcat(message, "send-path:");
                strcat(message, temp);
            }
            // We want to swap the toPort and fromPort fields
            else if(!strcmp(pairs.keys[i], "toPort"))
            {
                index_to_port = i;
                // Check to see if the index_from_port field was set.
                // If not, wait until fromPort has been processed
                if(index_from_port)
                {
                    // Swap the toPort and fromPort fields
                    strcpy(temp, pairs.values[index_from_port]);
                    strcpy(pairs.values[index_from_port], pairs.values[index_to_port]);
                    strcpy(pairs.values[index_to_port], temp);
                    
                    // Append the toPort field to the message
                    strcat(message, pairs.keys[index_to_port]);
                    strcat(message, ":");
                    strcat(message, pairs.values[index_to_port]);
                    strcat(message, " ");
                    // Append the fromPort field to the message
                    strcat(message, pairs.keys[index_from_port]);
                    strcat(message, ":");
                    strcat(message, pairs.values[index_from_port]);
                    strcat(message, " ");
                }
            }
            // We want to swap the toPort and fromPort fields
            else if(!strcmp(pairs.keys[i], "fromPort"))
            {
                index_from_port = i;
                // Check to see if the index_to_port field was set.
                // If not, wait until toPort has been processed
                if(index_to_port)
                {
                    // Swap the toPort and fromPort fields
                    strcpy(temp, pairs.values[index_from_port]);
                    strcpy(pairs.values[index_from_port], pairs.values[index_to_port]);
                    strcpy(pairs.values[index_to_port], temp);
                    
                    // Append the toPort field to the message
                    strcat(message, pairs.keys[index_to_port]);
                    strcat(message, ":");
                    strcat(message, pairs.values[index_to_port]);
                    strcat(message, " ");
                    // Append the fromPort field to the message
                    strcat(message, pairs.keys[index_from_port]);
                    strcat(message, ":");
                    strcat(message, pairs.values[index_from_port]);
                    strcat(message, " ");
                }
            }
            // We want to update the location to mine
            else if(!strcmp(pairs.keys[i], "location"))
            {
                sprintf(temp, "%d ", *my_location);
                strcat(message, "location:");
                strcat(message, temp);
            }
            // We don't want to add the location as is (we want to add ours).
            // Add the other fields as-is
            else if(strcmp(pairs.keys[i], "location"))
            {
                // Add the key at index i
                strcat(message, pairs.keys[i]);
                // Separate key and value by colon
                strcat(message, ":");
                // Add the value
                strcat(message, pairs.values[i]);
                // Add a space before next key-val pair
                strcat(message, " ");
            }
       }
    }
}

int is_an_ack() {
    // Iterate through all the pairs
    for(int i = 0; i < num_of_pairs; i++)
    {
        // If there's a pair with the type key
        if(!strcmp(pairs.keys[i], "type"))
            // Ensure the assoicated value is an ACK
            if(!strcmp(pairs.values[i], "ACK"))
                return 1;
    }
    return 0;
}

void send_to_drones(char message[], int sock_desc, struct Drone drones[], int* num_drones) {
    int rc;
    struct sockaddr_in to_address; /* address of drone to send to */
    
    // For each drone
    for (int i = 0; i < *num_drones; i++)
    {
        // Don't send to myself
        if (drones[i].port_number != port_number)
        {
            configure_address_struct(&to_address, drones[i].port_number, drones[i].drone_ip);
            // Send the message to the ith drone
            rc = send_message(sock_desc, to_address, message);
            // Check for any errors
            if (rc < 0)
            {
                printf("Issue sending message to drone.\n");
                exit(1);
            }
        }
    }
}

void receive_ack(struct Drone drones[], int* num_drones, int* my_location) {
    // Iterate through all the message pairs
    for(int i = 0; i < num_of_pairs; i++)
    {
        // Find the send-path
        if(!strcmp(pairs.keys[i], "send-path"))
        {
            char *token;
            // Make a copy of the path (strtok inserts null terminator at delim)
            char *path_copy = strdup(pairs.values[i]);
            // Grab the port number of the drone that send the ACK
            token = strtok(path_copy, ",");
            // Convert to an integer
            int port = atoi(token);
            int seq;
            // Find the sequence number associated with the message
            for(int j = 0; j < num_of_pairs; j++)
            {
                if(!strcmp(pairs.keys[j], "seqNumber"))
                {
                    // Convert the sequence number to an integer
                    seq = atoi(pairs.values[j]);
                    // Iterate through all the drones
                    for(int k = 0; k < *num_drones; k++)
                    {
                        // Find the drone that matches the first port in the msg
                        if(drones[k].port_number == port)
                        {
                            // Check if the sequence number is what we expect
                            if(drones[k].seq_num == seq)
                            {
                                // If so, print the message
                                print_pairs(*my_location);
                                printf("Type message:\n");
                                // Expecting seq_num++ in next message sent
                                // from the same drone
                                drones[k].seq_num++;
                            }
                        }
                    }
                }
            }
        }
    }
}

void is_expected_message(struct Drone drones[], int* num_drones, int* my_location) {
    int from;
    int seq;
    // Iterate through all pairs in the message
    for(int i = 0; i < num_of_pairs; i++)
    {
        // Find the sequence number assocated with the message
        if(!strcmp(pairs.keys[i], "seqNumber"))
            seq = atoi(pairs.values[i]);
        // Find the fromPort assoicated with the message
        else if(!strcmp(pairs.keys[i], "fromPort"))
            from = atoi(pairs.values[i]);
    }  
    // Iterate through all the drones
    for(int j = 0; j < *num_drones; j++)
    {
        // Find the drone that matches the fromPort
        if(drones[j].port_number == from)
        {
            // Only print the message if we haven't received
            // the message before (seq num for drone gets incremented with 
            // each message sent from that drone)
            if(drones[j].seq_num == seq)
            {
                print_pairs(*my_location);
                printf("Type message:\n");
                drones[j].seq_num++;
            }
        }
    }      
}

