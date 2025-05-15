# To compile all three programs, server all executable names here.
all: server

# Creates the executable "server" using the flag "-o".
server: server.o
	gcc server.o -o server -g -lm -Wall

# Creates the object file "server.o" using the flag "-c".
server.o: server.c
	gcc server.c -c -g -lm -Wall




# Delete object files and executatbles to start afresh.
# Be sure to delete the "palindrome" executable also.
# Delete these files BEFORE committing your work.
clean:
	rm -f *.o server
