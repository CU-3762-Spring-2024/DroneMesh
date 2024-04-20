#include "drone.h"

// Main driver
int main(int argc, char *argv[])
{
    struct Args *shared_vars = (struct Args *)malloc(sizeof(struct Args));
    int sd; /* the socket descriptor */
    struct sockaddr_in drone_address; /* my address */
    int rc; // always need to check return codes!
    int num_drones = 0;
    // Array of drones, can hold up to 50
    struct Drone drones[50];

    // Check to see if the right number of parameters was entered
    if (argc < 2)
    {
        printf ("usage is drone <port number>\n");
        exit(1); /* just leave if wrong number entered */
    }
    
    // Ask the user to specify the dimensions of the grid
    printf("Enter the number of columns in the grid: ");
    scanf("%d", &shared_vars->num_cols);
    printf("Enter the number of rows in the grid: ");
    scanf("%d", &shared_vars->num_rows);
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
    shared_vars->port_number = strtol(argv[1], NULL, 10);

    // Ensure port num is within correct range
    check_port_range(shared_vars->port_number);

    // Configure the drone_address struct
    configure_address_struct(&drone_address, shared_vars->port_number, NULL);

    char config_file_name[] = "config.file";
    // Read the config file first
    read_file(config_file_name, drones, &num_drones, shared_vars);

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
    listen_for_input(sd, drones, &num_drones, shared_vars);
}
// End of main