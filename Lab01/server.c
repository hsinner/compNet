//
// Created by hsinr on 4/2/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1025
#define EXIT_COMMAND ";;;"
#define BACKLOG 10

ssize_t send_all(int sockfd, const void *buf, size_t len);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, new_fd, udp_sock;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    char buf[MAXDATASIZE];
    int rv;
    int socktype;
    int message_count = 0;
    char s[INET6_ADDRSTRLEN];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s -t|-u <port number>\n", argv[0]);
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
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    if (socktype == SOCK_STREAM) {
        if (listen(sockfd, BACKLOG) == -1) {
            perror("listen");
            exit(1);
        }
        printf("Server is waiting for TCP connections on port %s...\n", argv[2]);

        while (1) {
            addr_len = sizeof client_addr;
            new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
            if (new_fd == -1) {
                perror("accept");
                continue;
            }
            inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
            printf("Server: got connection from %s\n", s);
            printf("*********************************************\n");
            printf("Now Listening for incoming messages...\n");
            while ((rv = recv(new_fd, buf, MAXDATASIZE-1, 0)) > 0) {

                buf[rv] = '\0';
                printf("Received: %s\n", buf);

                if (strcmp(buf, EXIT_COMMAND) == 0) {
                    printf("Client from %s has exited.\n", s);
                    break;
                }
                for (int i = 0; buf[i]; i++) {
                    buf[i] = toupper(buf[i]);
                }
                char response[MAXDATASIZE];
                snprintf(response, MAXDATASIZE, "%d %s", ++message_count, buf);

                printf("Now sending message %d back after changing back to Upper case...\n", message_count);

                // need to send all of them...
                send_all(new_fd, response, strlen(response));
                //send(new_fd, response, strlen(response), 0);
            }

            close(new_fd);
        }
    } else { // UDP mode
        printf("Server is waiting for UDP messages on port %s...\n", argv[2]);
        while (1) {
            addr_len = sizeof client_addr;
            rv = recvfrom(sockfd, buf, MAXDATASIZE-1, 0, (struct sockaddr *)&client_addr, &addr_len);
            if (rv == -1) {
                perror("recvfrom");
                continue;
            }
            buf[rv] = '\0';
            inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
            printf("Received from %s: %s\n", s, buf);
        }
    }

    close(sockfd);
    return 0;
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