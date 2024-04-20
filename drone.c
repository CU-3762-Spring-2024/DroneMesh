#include "drone.h"
#define STDIN 0
#define MSG 0
#define MOV 1
#define MAX_LEN 512

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

void read_file(char file_name[], struct Drone drones[], int* num_drones, struct Args *args) {
    
    size_t len = 0; /* size of the line buffer */
    char *line = NULL;
    FILE* stream = open_file(file_name);

    // Extract each line from the file
    while(getline(&line, &len, stream) != -1)
    {
        char *token;
        // Fields separated by space or newline char
        const char delimiter[] = " \n";
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
        // If it's my port
        if(args->port_number == drone_port)
        {
            // Record my location
            args->my_location = strtol(token, NULL, 10);
            printf("Location is %d\n", args->my_location);
            // Calculate my coordinates
            args->my_coordinates = calculate_coordinates(args->my_location, args);
        }
        // in_path used to keep track of drones already in send-path.
        // This ensures that we won't resend message to drone that's 
        // already received the message
        drones[*num_drones].in_path = 0;
        // The sequence number for each drone will start at 1
        drones[*num_drones].seq_num = 1;
        // Keep track of the number of drones
        (*num_drones)++;
    }
    // Ensure the file is closed when finished
    fclose(stream);
}

int send_message(int sock_desc, struct sockaddr_in drone_addr, char msg[])
{
    int rc;
    char buffer_out[MAX_LEN];
    // Clear the buffer
    memset(buffer_out, 0, MAX_LEN);
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
    for(int i = 0; i < strlen(port_num); i++)
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

void receive_message(int sock_desc, struct Drone drones[], int* num_drones, struct Args *args) {
    int rc;
    struct sockaddr_in from_address;  /* address of sender */
    char buffer_received[MAX_LEN]; // used in recvfrom
    int flags = 0; // used for recvfrom
    socklen_t from_length;

    // from_length must have an initial value
    from_length = sizeof(struct sockaddr_in);
    // Assume the message isn't for me until I confirm the port
    // number in the message
    args->is_my_message = 0;
    // Clear the buffer in each iteration
    memset (buffer_received, 0, MAX_LEN);
    rc = recvfrom(sock_desc, buffer_received, MAX_LEN, flags,
                  (struct sockaddr *) &from_address, &from_length);
    // Exit if there was an issue with the receive
    if(rc < 0)
    {
        perror("Issue receiving bytes.\n");
        exit(1);
    }
    // parse_text will call yylex() to extract key-value pairs
    // from the message and take the appropriate action
    parse_text(buffer_received, args);
    // Process the message based on the message type
    process_msg_type(buffer_received, sock_desc, drones, num_drones, args);
}

/* Note: the lex scanner requires input from the console or from a file.
 * We set the variable yyin to point to a file which tells lex we want it
 * to read input from this file rather than the console. Also, with the lex
 * scanner, it wouldn't allow me to open the file in read/write mode so I had
 * to open the file in write mode to write to the file, then open it again in
 * read mode for the scanner itself
 * */
void parse_text(char msg[], struct Args *args) {
    // Extern variables defined in lex.yy.c
    extern int yylex();
    extern FILE *yyin;
    
    // Each instance of the server needs its own
    // unique file name (if it's ran on the same machine), so the port number is
    // used to create a unique file name for the drone. This is necessary as the lex scanner needs
    // each line sent from the client to be written to a file for analysis. If each instance
    // of the server uses the same file this will cause incorrect results since they
    // are attempting to read and write to the same file simultaneously
    char file[10];
    sprintf(file, "%d", args->port_number);
    // Add the .txt extension to the file name
    strcat(file, ".txt");
    
    // Open the file in write mode
    yyin = fopen(file, "w");
    // Print contents of message to file
    fprintf(yyin, "%s", msg);
    fclose(yyin);

    // Open the file in read mode
    yyin = fopen(file, "r");
    // Pass file to the scanner for analysis
    yylex(args);
    fclose(yyin);
}

void format_pair(char* key_val_pair, struct Args *args) {
    char *token;
    const char delimiter[] = ":";

    // Extract token delimited by the colon
    token = strtok(key_val_pair, delimiter);
    // Copy the first token (key) to pairs.keys struct
    strcpy(args->pairs.keys[args->num_of_pairs], token);
    // Get the next token (value)
    token = strtok(NULL, delimiter);
    // Copy the token to pairs.values struct
    strcpy(args->pairs.values[args->num_of_pairs], token);
    args->num_of_pairs++;
}

void print_pairs(struct Args *args) {
    // Print all key-value pairs
    printf("\n\t\t    ------------------------------------------\n");
    printf("\t\t%20s", "myLocation");
    printf("%20d\n", args->my_location);
    for(int i = 0; i < args->num_of_pairs; i++)
    {
        // Print the key
        printf("\t\t%20s", args->pairs.keys[i]);
        // Print the value
        printf("%20s\n", args->pairs.values[i]);
    }
    printf("\t\t    ------------------------------------------\n");
}

void create_reg_message(char msg[], char* line, char port[], struct Args *args, struct Drone drones[], int* num_drones) {
    char tmp[20];
    // Clear the msg buffer
    memset(msg, 0, MAX_LEN);
    
    strcat(msg, "msg:\"");
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
    sprintf(tmp, "%d", args->port_number);
    strcat(msg, tmp);
    strcat(msg, " ");

    // Add the version field
    strcat(msg, "version:7 ");

    // Add the location field
    strcat(msg, "location:");
    // Convert location to a string and store in tmp
    sprintf(tmp, "%d", args->my_location);
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
    sprintf(tmp, "%d ", args->port_number);
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


struct Coordinates calculate_coordinates(int location, struct Args *args) { 
    int row_num = -1;
    int col_num;
    /* Continuously subtract the number of columns from the grid location. Each time
     * we subract the number of columns, increment the row number. In the last iteration,
     * once location <= num_cols we record the col_num and the loop exits */
    while(location > 0)
    {
        // Index starts from 0 so we subtract 1
        col_num = location - 1;  
        location -= args->num_cols;  
        row_num++;
    }
    struct Coordinates coordinates;
    // Save the x and y coordinate in the struct
    coordinates.x = col_num;
    coordinates.y = row_num;
    return coordinates;
}

void listen_for_input(int sock_desc, struct Drone drones[], int* num_drones, struct Args *args) {
    // For the select
    fd_set socket_fds; // The socket descriptor set
    int max_sd; // Tells the OS how many sockets are set
    int rc;

    input_msg();
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
            create_message(sock_desc, drones, num_drones, args);
        // If a message is being sent over the network
        if(FD_ISSET(sock_desc, &socket_fds))
            // Start receiving message from the server
            receive_message(sock_desc, drones, num_drones, args);
    }
}

void modify_fields(char message[], struct Args *args) {
    memset(message, 0, MAX_LEN);
    for(int i = 0; i < args->num_of_pairs; i++)
    {
        char temp[20];
        // Evaluates to true if key matches 'send-path'
        if(!strcmp(args->pairs.keys[i], "send-path")) 
        {
            // Add port number to send path
            sprintf(temp, ",%d ", args->port_number);
            strcat(args->pairs.values[i], temp);
            strcat(message, args->pairs.keys[i]);
            strcat(message, ":");
            strcat(message, args->pairs.values[i]);
            continue;
        }
        // Evaluates to true if key matches 'TTL'
        else if(!strcmp(args->pairs.keys[i], "TTL"))
        {
            int ttl = atoi(args->pairs.values[i]);
            ttl--;
            sprintf(temp, "%d", ttl);
            strcpy(args->pairs.values[i], temp);
            strcat(message, args->pairs.keys[i]);
            strcat(message, ":");
            strcat(message, args->pairs.values[i]);
            strcat(message, " ");
            continue;
        }
        // Evaluates to true if key matches 'location'
        else if(!strcmp(args->pairs.keys[i], "location"))
        {
            sprintf(temp, "%d ", args->my_location);
            strcpy(args->pairs.values[i], temp);
            strcat(message, args->pairs.keys[i]);
            strcat(message, ":");
            strcat(message, args->pairs.values[i]);
            strcat(message, " ");
            continue;
        }
        else
        {
            // Append the other fields without any changes
            strcat(message, args->pairs.keys[i]);
            strcat(message, ":");
            strcat(message, args->pairs.values[i]);
            strcat(message, " ");
        }
    }
}

// Return 0 if false and 1 if true
int has_message(struct Args *args) {
    // Iterate through all the pairs
    for(int i = 0; i < args->num_of_pairs; i++)
        // If the message has a msg field
        if(!strncmp(args->pairs.keys[i], "msg", 3))
            return 1;
    return 0;
}

void create_ack_message(char message[], struct Args *args) {
    // Clear the message
    memset(message, 0, MAX_LEN);
    // Add the type to message
    strcpy(message, "type:ACK ");
    char temp[10];
    int index_from_port = 0;
    int index_to_port = 0;
    for(int i = 0; i < args->num_of_pairs; i++)
    {
       // If the key is not 'msg'
       if(strcmp(args->pairs.keys[i], "msg"))
       {
            // Update the send-path by adding my port
            if(!strcmp(args->pairs.keys[i], "send-path"))
            {
                char temp[18];
                sprintf(temp, "%d ", args->port_number);
                strcat(message, "send-path:");
                strcat(message, temp);
            }
            // We want to swap the toPort and fromPort fields
            else if(!strcmp(args->pairs.keys[i], "toPort"))
            {
                index_to_port = i;
                // Check to see if the index_from_port field was set.
                // If not, wait until fromPort has been processed
                if(index_from_port)
                {
                    // Swap the toPort and fromPort fields
                    strcpy(temp, args->pairs.values[index_from_port]);
                    strcpy(args->pairs.values[index_from_port], args->pairs.values[index_to_port]);
                    strcpy(args->pairs.values[index_to_port], temp);
                    
                    // Append the toPort field to the message
                    strcat(message, args->pairs.keys[index_to_port]);
                    strcat(message, ":");
                    strcat(message, args->pairs.values[index_to_port]);
                    strcat(message, " ");
                    // Append the fromPort field to the message
                    strcat(message, args->pairs.keys[index_from_port]);
                    strcat(message, ":");
                    strcat(message, args->pairs.values[index_from_port]);
                    strcat(message, " ");
                }
            }
            // We want to swap the toPort and fromPort fields
            else if(!strcmp(args->pairs.keys[i], "fromPort"))
            {
                index_from_port = i;
                // Check to see if the index_to_port field was set.
                // If not, wait until toPort has been processed
                if(index_to_port)
                {
                    // Swap the toPort and fromPort fields
                    strcpy(temp, args->pairs.values[index_from_port]);
                    strcpy(args->pairs.values[index_from_port], args->pairs.values[index_to_port]);
                    strcpy(args->pairs.values[index_to_port], temp);
                    
                    // Append the toPort field to the message
                    strcat(message, args->pairs.keys[index_to_port]);
                    strcat(message, ":");
                    strcat(message, args->pairs.values[index_to_port]);
                    strcat(message, " ");
                    // Append the fromPort field to the message
                    strcat(message, args->pairs.keys[index_from_port]);
                    strcat(message, ":");
                    strcat(message, args->pairs.values[index_from_port]);
                    strcat(message, " ");
                }
            }
            // We want to update the location to mine
            else if(!strcmp(args->pairs.keys[i], "location"))
            {
                sprintf(temp, "%d ", args->my_location);
                strcat(message, "location:");
                strcat(message, temp);
            }
            // We don't want to add the location as is (we want to add ours).
            // Add the other fields as-is
            else if(strcmp(args->pairs.keys[i], "location"))
            {
                // Add the key at index i
                strcat(message, args->pairs.keys[i]);
                // Separate key and value by colon
                strcat(message, ":");
                // Add the value
                strcat(message, args->pairs.values[i]);
                // Add a space before next key-val pair
                strcat(message, " ");
            }
        }
    }
}

int is_an_ack(struct Args *args) {
    // Iterate through all the pairs
    for(int i = 0; i < args->num_of_pairs; i++)
    {
        // If there's a pair with the type key
        if(!strcmp(args->pairs.keys[i], "type"))
            // Ensure the assoicated value is an ACK
            if(!strcmp(args->pairs.values[i], "ACK"))
                return 1;
    }
    return 0;
}

void receive_ack(struct Drone drones[], int* num_drones, struct Args *args) {
    // Iterate through all the message pairs
    for(int i = 0; i < args->num_of_pairs; i++)
    {
        // Find the send-path
        if(!strcmp(args->pairs.keys[i], "send-path"))
        {
            char *token;
            // Make a copy of the path (strtok inserts null terminator at delim)
            char *path_copy = strdup(args->pairs.values[i]);
            // Grab the port number of the drone that send the ACK
            token = strtok(path_copy, ",");
            // Convert to an integer
            int port = atoi(token);
            int seq;
            // Find the sequence number associated with the message
            for(int j = 0; j < args->num_of_pairs; j++)
            {
                if(!strcmp(args->pairs.keys[j], "seqNumber"))
                {
                    // Convert the sequence number to an integer
                    seq = atoi(args->pairs.values[j]);
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
                                print_pairs(args);
                                input_msg();
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

int is_expected_message(struct Drone drones[], int* num_drones, struct Args *args) {
    int from;
    int seq;
    // Iterate through all pairs in the message
    for(int i = 0; i < args->num_of_pairs; i++)
    {
        // Find the sequence number assocated with the message
        if(!strcmp(args->pairs.keys[i], "seqNumber"))
            seq = atoi(args->pairs.values[i]);
        // Find the fromPort assoicated with the message
        else if(!strcmp(args->pairs.keys[i], "fromPort"))
            from = atoi(args->pairs.values[i]);
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
                print_pairs(args);
                input_msg();
                drones[j].seq_num++;
                return 1;
            }
        }
    } 
    return 0;     
}

void input_msg() {
    printf("\nYou can either type a message or a move command. In the case\n"
           "of a message, you can simply type the message. If you'd like\n"
           "to have a drone move to a different location, type the location\n"
           "(integer value).\n");
    printf(">> ");
    fflush(STDIN);
}

void create_message(int sock_desc, struct Drone drones[], int* num_drones, struct Args *args) {
    size_t len = 0; /* size of the line buffer */
    char *line = NULL;
    // Read the line from console and continue if no errors
    if(getline(&line, &len, stdin) != -1)
    {
        // Remove the new line character
        line[strlen(line) - 1] = '\0';
        char msg[MAX_LEN];
        int user_choice;
        // Prompt the user to enter destination port. This value
        // will be appended to the message send to each drone in the network
        char destination_port[6];
        printf("What port number is this message destined for?\n>> ");
        scanf("%s", destination_port);
        printf("Is this a message (0) or a move command (1)?\n>> ");
        scanf("%d", &user_choice);
        // Consume the newline character (when user presses enter)
        getchar();
        // 2 different cases based on the user_choice
        switch(user_choice)
        {
            // If the user selected to send a message
            case(MSG):
            {
                create_reg_message(msg, line, destination_port, args, drones, num_drones);
                // Send message to all the drones in the network
                send_to_drones(msg, sock_desc, drones, num_drones, args);
                input_msg();
                break;
            }
            // If the user selected to send a move commmand
            case(MOV):
            {
                int rc;
                struct sockaddr_in to_address;
                create_reg_message(msg, line, destination_port, args, drones, num_drones);
                strcat(msg, " move:");
                // Append the location that we want the drones to move to
                strcat(msg, line);
                for(int i = 0; i < *num_drones; i++)
                {
                    // Send to specific drone only
                    if(drones[i].port_number == atoi(destination_port))
                    {
                        configure_address_struct(&to_address, drones[i].port_number, drones[i].drone_ip);
                        rc = send_message(sock_desc, to_address, msg);
                        if (rc < 0)
                        {
                            printf("Issue sending message to drone.\n");
                            exit(1);
                        }
                    }
                }
                input_msg();
                break;
            }
            // Invalid user_choice
            default:
            {
                printf("Invalid message type. Exiting program now\n");
                exit(1);
            }
        }
    }
    else
    {
        printf("Error reading from console!\n");
        exit(1);
    }
}

void send_to_drones(char message[], int sock_desc, struct Drone drones[], int* num_drones, struct Args *args) {
    int rc;
    struct sockaddr_in to_address; /* address of drone to send to */
    // Iterate through all the drones
    for(int i = 0; i < *num_drones; i++)
    {
        // Only send to the drones that aren't already in the send path
        if(!drones[i].in_path && drones[i].port_number != args->port_number)   
        {  
            configure_address_struct(&to_address, drones[i].port_number, drones[i].drone_ip);
            rc = send_message(sock_desc, to_address, message);
            if (rc < 0)
            {
                printf("Issue sending message to drone.\n");
                exit(1);
            }
        }
        // Reset the in_path
        drones[i].in_path = 0;
    }
}

void process_msg_type(char message[], int sock_desc, struct Drone drones[], int* num_drones, struct Args *args) {
    /* Four cases:
     * 1) If the message is a move command, update my location and print a confirmation message
     * 2) If the message is meant for me, is in range and still alive, print the message and send an ACK to every drone
     * 3) If the message for me is an ACK sent from another drone (from a message I sent), print the message but dont resend
     * 4) If the message is meant for another drone, is in range and still alive,
     *    forward the message to every drone in the network that hasn't received the message (before sending the 
     *    message, replace the fromLocation field with my location and decrement the TTL field. */ 
    
    // Case for move command
    if(args->is_move_cmd)
    {
        printf("Moving to location %d\n", args->my_location);
        // Recalculate coordinates based on new location
        args->my_coordinates = calculate_coordinates(args->my_location, args);
        // Reset for the next message
        args->is_move_cmd = 0;
        input_msg();
    }
    // Case if the message is meant for me and is still in range and alive
    else if(args->is_my_message && args->is_in_range && args->is_alive)
    {
        // If receiving an ACK message for previously sent message
        if(is_an_ack(args))
        {
            // Process the ACK message
            receive_ack(drones, num_drones, args);
        }
        // Else the message is not an ACK message
        else
        {
            // Process the (non-ACK) message and be sure it's what we expect
            // based on the sequence number for that particular drone
            if(is_expected_message(drones, num_drones, args))
            {
                // If the message sent contains a msg field, 
                // an ACK needs to be sent to the sending drone
                if(has_message(args))
                {
                    // Create ACK fields
                    create_ack_message(message, args);
                    send_to_drones(message, sock_desc, drones, num_drones, args);
                }
            }
        }
    }
    // If the message isn't for me but is still in range and alive
    else if(!args->is_my_message && args->is_in_range && args->is_alive)
    {
        // Update the fields in the message with my information
        modify_fields(message, args);
        for(int i = 0; i < args->num_of_pairs; i++)
        {
            // Find the send-path key in the pairs
            if(!strcmp(args->pairs.keys[i], "send-path"))
            {
                char *token;
                // Extract the first port number in the send-path
                token = strtok(args->pairs.values[i], ",");
                // While there's still a port number
                while(token != NULL)
                {
                    // Iterate through all the drones
                    for(int j = 0; j < *num_drones; j++)
                        // Find the drone that matches to port number in the token
                        if(drones[j].port_number == atoi(token))
                            // Drone is in the path already
                            drones[j].in_path = 1;
                    // Grab the next port nuber
                    token = strtok(NULL, args->pairs.keys[i]);
                }
            }
        }
        // Forward the message
        send_to_drones(message, sock_desc, drones, num_drones, args);
    }
    // Clear the struct storing the key-value pairs by
    // setting num_of_pairs to 0. This will ensure the previous
    // data will get overwritten when the next line is received
    // and processed
    args->num_of_pairs = 0;
}


