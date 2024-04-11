FLEX = /usr/bin/flex
CC = gcc
OBJCS = drone.c
CFLAGS = -g -Wall
LIBS = -lm # Add your libraries if needed

all: drone

drone: drone.o parse.o
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

parse.c: parse.l
	$(FLEX) $(LFLAGS) $<
	mv lex.yy.c $@

parse.o: parse.c
	$(CC) -c $(CFLAGS) $< 2>/dev/null

clean:
	rm -f drone parse.o parse.c drone.o
