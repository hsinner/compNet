# Target to build everything
all: proxy

# Build the proxy executable from proxy.o
proxy: proxy.o
	gcc proxy.o -o proxy -g -lm -Wall

# Compile proxy.c into proxy.o
proxy.o: proxy.c
	gcc -c proxy.c -o proxy.o -g -Wall

# Clean up object files and executables
clean:
	rm -f *.o proxy