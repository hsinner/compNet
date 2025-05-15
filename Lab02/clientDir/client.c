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
#include <libgen.h>

#include <arpa/inet.h>

#define PORT "3490"
#define EXIT_COMMAND ";;;"
#define MAXDATASIZE 1025

bool readLine(char** line, size_t* size, size_t* length);
bool prompt_directory(char** dir);
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
    while (readLine(&line, &size, &len)) {
        printf("Client read: %s\n", line);

        if (strncmp(line, "iWant ", 6) == 0) {
            if (socktype != SOCK_STREAM) {
                printf("'iWant' command is only supported over TCP.\n");
                continue;
            }

            char* filename = line + 6;

            if (send_all(sockfd, line, len) < 0) {
                perror("client: send");
                continue;
            }
            char* dir_path = NULL;
            if (!prompt_directory(&dir_path)) {
                fprintf(stderr, "Failed to get directory input.\n");
                continue;
            }

            char* basename_filename = basename(filename);

            char fullpath[MAXDATASIZE];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, basename_filename);

            uint32_t net_size;
            int bytes = recv(sockfd, &net_size, sizeof(net_size), 0);
            if (bytes != sizeof(net_size)) {
                fprintf(stderr, "Failed to receive file size.\n");
                continue;
            }
            long filesize = ntohl(net_size);

            FILE* fp = fopen(fullpath, "wb");
            if (!fp) {
                perror("fopen");
                continue;
            }

            long total_received = 0;
            while (total_received < filesize) {
                bytes = recv(sockfd, buf, MAXDATASIZE, 0);
                if (bytes <= 0) break;
                fwrite(buf, 1, bytes, fp);
                total_received += bytes;
            }
            fclose(fp);
            printf("File '%s' received (%ld bytes) and saved to '%s'.\n", filename, total_received, fullpath);
        }

        else if (strncmp(line, "uTake ", 6) == 0) {
            if (socktype != SOCK_STREAM) {
                printf("'uTake' command is only supported over TCP.\n");
                continue;
            }

            char* filename = line + 6;

            char* dir_path = NULL;
            if (!prompt_directory(&dir_path)) {
                fprintf(stderr, "Failed to get directory input.\n");
                continue;
            }

            char* basename_filename = basename(filename);

            char fullpath[MAXDATASIZE];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, basename_filename);

            char sendline[MAXDATASIZE];
            snprintf(sendline, sizeof(sendline), "uTake %s", fullpath);

            size_t sendlen = strlen(sendline);
            if (send_all(sockfd, sendline, sendlen) < 0) {
                perror("client: send");
                continue;
            }

            FILE* fp = fopen(filename, "rb");
            if (!fp) {
                printf("File '%s' not found.\n", filename);
                continue;
            }
            printf("Reading file from: %s\n", filename);



            fseek(fp, 0, SEEK_END);
            long filesize = ftell(fp);
            rewind(fp);

            uint32_t net_size = htonl((uint32_t)filesize);
            send_all(sockfd, &net_size, sizeof(net_size));

            while (!feof(fp)) {
                size_t bytes_read = fread(buf, 1, MAXDATASIZE, fp);
                if (bytes_read > 0)
                    send_all(sockfd, buf, bytes_read);
            }
            fclose(fp);
            printf("Sent file '%s' (%ld bytes)\n", filename, filesize);
        }

        else if (socktype == SOCK_STREAM) {
            if (send_all(sockfd, line, len) < 0) {
                perror("client: send");
                continue;
            }

            if (strcmp(line, EXIT_COMMAND) == 0) {
                printf("Exit command received. Closing client.\n");
                break;
            }

            int numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);
            if (numbytes == -1) {
                perror("client: recv");
                continue;
            }
            buf[numbytes] = '\0';
            printf("Server response: %s\n", buf);
        }

        else {
            // For UDP, send as-is if it's not iWant/uTake
            if (sendto(sockfd, line, len, 0, (struct sockaddr *)&server_addr, p->ai_addrlen) == -1) {
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

bool prompt_directory(char** dir) {
    while (1) {
        // Prompt the user for a directory input
        printf("Enter the directory to save the file : ");

        size_t len = 0;
        if (!readLine(dir, &len, &len)) {
            printf("Failed to read directory.\n");
            return false;  // Failed to read input
        }

        if ((*dir)[len - 1] == '\n') {
            (*dir)[len - 1] = '\0';
        }

        if (len == 0) {
            printf("Directory cannot be empty. Please enter a valid directory.\n");
            continue;
        }

        return true;
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