//
// Created by hsinr on 4/2/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the DEFAULT port client will be connecting to
#define EXIT_COMMAND ";;;"
#define MAXDATASIZE 1025 // max number of bytes we can get at once

bool readLine(char** line, size_t* size, size_t* length);
ssize_t send_all(int sockfd, const void *buf, size_t len);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	int socktype;
    struct sockaddr_storage server_addr;
    socklen_t addr_len = sizeof(server_addr);

	if (argc != 4) {
	    fprintf(stderr,"usage: client -t|-u <server IP address> <port number>\n");
	    exit(1);
	}

    if (strcmp(argv[1], "-t") == 0) {
        socktype = SOCK_STREAM;  // TCP mode
    } else if (strcmp(argv[1], "-u") == 0) {
        socktype = SOCK_DGRAM;  // UDP mode
    } else {
        fprintf(stderr, "Invalid mode. Use -t for TCP or -u for UDP.\n");
        exit(1);
    }


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;

    if((rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    //loop through list, connect to first possible
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd == -1) {
            perror("client: socket");
            continue;
        }

        // TCP
        if (socktype == SOCK_STREAM) {
        	rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
        	if(rv == -1) {
            	perror("client: connect");
            	continue;
        	}
        // UDP
        } else {
        	memcpy(&server_addr, p->ai_addr, p->ai_addrlen);
            addr_len = p->ai_addrlen;
        }


        //made it here... all is well;
        break;
    }


    if(p == NULL) {
        fprintf(stderr, "client: failed to connect... now exiting\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);

    printf("Client has connected to %s\n", s);

    freeaddrinfo(servinfo);

    char* line = NULL;
    size_t size = 0;
    size_t len;

    while(readLine(&line, &size, &len)) {
        printf("Client read: %s\n", line);
        if (socktype == SOCK_STREAM) {
          	if (send_all(sockfd, line, len) < 0) {
            	perror("client: send");
          	}
            if (strcmp(line, EXIT_COMMAND) == 0) {
                printf("Exit command received. Closing client.\n");
                break;
            }
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
                perror("client: recv");
                continue;
            }
            buf[numbytes] = '\0';
            printf("Server response: %s\n", buf);
        } else {
        	if(sendto(sockfd, line, len, 0, (struct sockaddr *)&server_addr, p->ai_addrlen) == -1) {
            	perror("client: sendto");
        	}
            if (strcmp(line, EXIT_COMMAND) == 0) {
                printf("Exit command received. Closing client.\n");
                break;
            }
        }
    }
    printf("*******************************************\n");
    printf("Attempting to shutdown client\n");
    close(sockfd);
    printf("Successful Shutdown... Bye\n");


	return 0;
}

bool readLine(char** line, size_t* size, size_t* length)
{
    while(1) {
        //get line
        printf("string for server > ");
        size_t len = getline(line, size, stdin);

        //handle EOF
        if(len == -1)
            return false;

        //Strip off trailing '\n'
        if((*line)[len-1] == '\n')
            (*line)[--len] = '\0';

        *length = len;

        if(len == 0)
            continue;

        return len > 1 || **line != '.';
    }
}

ssize_t send_all(int sockfd, const void *buf, size_t len) {
    ssize_t total_sent = 0;
    while (total_sent < len) {
        ssize_t bytes_sent = send(sockfd, buf + total_sent, len - total_sent, 0);
        if (bytes_sent == -1) {
            perror("send error");
            return -1;  // Return error if send fails
        }
        total_sent += bytes_sent;
    }
    return total_sent;
}