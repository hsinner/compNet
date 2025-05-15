# To compile all three programs, server all executable names here.
all: client server

# Creates the executable "client" using the flag "-o".
client: client.o
	gcc client.o -o client -g -lm -Wall

# Creates the object file "client.o" using the flag "-c".
client.o: client.c
	gcc client.c -c -g -lm -Wall



# Delete object files and executatbles to start afresh.
# Be sure to delete the "palindrome" executable also.
# Delete these files BEFORE committing your work.
clean:
	rm -f *.o client
