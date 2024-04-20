%{
	#include "drone.h"
	#include <math.h>
    // Redefine yylex() to take an Args struct as argument from drone.c
    #define YY_DECL int yylex(struct Args *args)
%}

invalid [A-Za-z]+:[A-Za-z0-9-]+:
version (version):(7)
toPort (toPort):[0-9]+
fromPort (fromPort):[0-9]+
ttl TTL:[0-9]+
flags (flags):[0-1]
message (msg):\"[A-Za-z0-9 '!.,?\-"]+\"
location (location):[0-9]+
sendPath (send-path):[0-9,]+
time (time):[0-9]+
seqNumber (seqNumber):[0-9]+
type (type):(ACK)
move (move):[0-9]+
eol \n
blanks [ \t]
unknown .

%%

{invalid} {;}
{version} {format_pair(yytext, args);}
{toPort} {
            int temp;
            format_pair(yytext, args);
            // Grab the port number that was just saved in the pairs struct
            temp = strtol(args->pairs.values[args->num_of_pairs - 1], NULL, 10);
            // Check to see if the port from the message matches my port
            if(args->port_number == temp)
            // Set flag if the message is mine
            args->is_my_message = 1;
}
{fromPort} {format_pair(yytext, args);}
{ttl} {
            int ttl;
            format_pair(yytext, args);
            ttl = strtol(args->pairs.values[args->num_of_pairs - 1], NULL, 10);
            args->is_alive = (ttl <= 0) ? 0 : 1;
}
{flags} {format_pair(yytext, args);}
{message}  {format_pair(yytext, args);}
{location} {
                int from_location;
                format_pair(yytext, args);
                // Grab the location of the drone sending the message
                from_location = strtol(args->pairs.values[args->num_of_pairs - 1], NULL, 10);
                struct Coordinates from_coordinates = calculate_coordinates(from_location, args);
                // Calculate how far away the drone is in the x and y direction
                int y = from_coordinates.y - args->my_coordinates.y;
                int x = from_coordinates.x - args->my_coordinates.x;
                // To find how many grid squares separate me and the sending drone
                int distance = sqrt(pow(y, 2) + pow(x, 2));
                // Out of range if more than 2 squares away
                args->is_in_range = (distance > 2) ? 0 : 1;
}
{sendPath} {format_pair(yytext, args);}
{time} {format_pair(yytext, args);}
{seqNumber} {format_pair(yytext, args);}
{type} {format_pair(yytext, args);}   
{move} {
            format_pair(yytext, args);
            args->my_location = strtol(args->pairs.values[args->num_of_pairs - 1], NULL, 10);
            args->is_move_cmd = 1;
}          
{eol} {;}
{blanks} {;}
{unknown} {;}

%%

int yywrap(void) { return 1; }
