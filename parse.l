%{
	    #include "drone.h"
	    #include <math.h>
	    extern int port_number;
	    extern int is_my_message;
	    extern int num_of_pairs;
	    extern struct Pairs pairs;
	    extern struct Coordinates my_coordinates;
    	extern int num_rows;
    	extern int num_cols;
    	extern int is_in_range;
    	extern int is_alive;
%}

invalid [A-Za-z]+:[A-Za-z0-9-]+:
version (version):(6)
toPort (toPort):[0-9]+
fromPort (fromPort):[0-9]+
ttl TTL:[0-9]+
flags flags:[0-1]
message msg:\"[A-Za-z0-9 '!.,?\-"]+\"
location (location):[0-9]+
sendPath (send-path):[0-9,]+
time (time):[0-9]+
seqNumber (seqNumber):[0-9]+
type (type):(ACK)

eol \n
blanks [ \t]
unknown .

%%

{invalid} {;}
{version} {format_pair(yytext);}
{toPort} {
            int temp;
            format_pair(yytext);
            // Grab the port number that was just saved in the pairs struct
            temp = strtol(pairs.values[num_of_pairs - 1], NULL, 10);
            // Check to see if the port from the message matches my port
            if(port_number == temp)
            {
                // Set flag if the message is mine
                is_my_message = 1;
            }
}
{fromPort} {format_pair(yytext);}
{ttl} {
            int ttl;
            format_pair(yytext);
            ttl = strtol(pairs.values[num_of_pairs - 1], NULL, 10);
            is_alive = (ttl <= 0) ? 0 : 1;
}
{flags} {format_pair(yytext);}
{message}  {format_pair(yytext);}
{location} {
                int from_location;
                format_pair(yytext);
                // Grab the location of the drone sending the message
                from_location = strtol(pairs.values[num_of_pairs - 1], NULL, 10);
                struct Coordinates from_coordinates = calculate_coordinates(from_location);
                // Calculate how far away the drone is in the x and y direction
                int y = from_coordinates.y - my_coordinates.y;
                int x = from_coordinates.x - my_coordinates.x;
                // To find how many grid squares separate me and the sending drone
                int distance = sqrt(pow(y, 2) + pow(x, 2));
                // Out of range if more than 2 squares away
                is_in_range = (distance > 2) ? 0 : 1;
}
{sendPath} {format_pair(yytext);}
{time} {format_pair(yytext);}
{seqNumber} {format_pair(yytext);}
{type} {
                format_pair(yytext);}
                
{eol} {;}
{blanks} {;}
{unknown} {;}

%%

int yywrap(void) { return 1; }
