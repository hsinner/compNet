//
// Created by hsinr on 5/7/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXDATASIZE 4096
#define MAX_PENDING 100

int listen_fd = -1;

void handle_client(int client_fd);
void send_error(int client_fd, int code, const char *reason, const char *msg);
void parse_url(const char *url, char *host, char *path, int *port);

void sigint_handler(int sig) {
    printf("\nServer shutting down gracefully...\n");

    if (listen_fd != -1) {
        close(listen_fd);  // Close the listening socket
    }

    exit(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    char buf[MAXDATASIZE];
    int rv;
    int message_count = 0;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_fd);
            perror("server: bind");
            continue;
        }

        if (listen(listen_fd, MAX_PENDING) < 0) {
            perror("server: listen");
            exit(1);
        }
        break;
    }

    printf("Proxy server listening on port %s\n", argv[1]);

    while (1) {
        addr_len = sizeof(client_addr);
        new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
        printf("Server: got connection from %s\n", s);

        pid_t pid = fork();
        if (pid == 0) {
            close(listen_fd);
            handle_client(new_fd);
            close(new_fd);
            exit(0);
        }   else if (pid > 0) {
            close(new_fd);

            int status;
            waitpid(pid, &status, WNOHANG);
        }   else {
            perror("fork");
        }
    }

    return 0;
}

void handle_client(int client_fd) {
    char buf[MAXDATASIZE], method[16], url[2048], version[16];
    ssize_t n = recv(client_fd, buf, MAXDATASIZE-1, 0);
    if (n <= 0) return;
    buf[n] = '\0';

    sscanf(buf, "%s %s %s", method, url, version);

    printf("Client requested URL: %s\n", url);

    if (strcmp(method, "GET") != 0) {
        send_error(client_fd, 501, "Not Implemented", "Only GET method is supported");
        return;
    }

    char *host_hdr = strstr(buf, "Host:");
    if (!host_hdr) {
        send_error(client_fd, 400, "Bad Request", "Missing host header");
        return;
    }

    char host[256], path[2048];
    int port = 80;
    parse_url(url, host, path, &port);

    struct hostent *server = gethostbyname(host);
    if (!server) {
        send_error(client_fd, 502, "Bad Gateway", "Failed to resolve hostname.");
        return;
    }

    int remote_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in remote_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };
    memcpy(&remote_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(remote_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        send_error(client_fd, 502, "Bad Gateway", "Could not connect to remote host.");
        close(remote_fd);
        return;
    }

    char request[MAXDATASIZE];
    snprintf(request, sizeof(request),
             "GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",
             path, version, host);
    send(remote_fd, request, strlen(request), 0);

    while ((n = recv(remote_fd, buf, MAXDATASIZE, 0)) > 0) {
        send(client_fd, buf, n, 0);
    }

    close(remote_fd);

}

void parse_url(const char *url, char *host, char *path, int *port) {
    char *p, *phost;
    if (strncmp(url, "http://", 7) == 0) {
        phost = (char *)url + 7;
    } else {
        phost = (char *)url;
    }

    p = strchr(phost, '/');
    if (p) {
        strcpy(path, p);
        *p = '\0';
    } else {
        strcpy(path, "/");
    }

    char *port_ptr = strchr(phost, ':');
    if (port_ptr) {
        *port_ptr = '\0';
        strcpy(host, phost);
        *port = atoi(port_ptr + 1);
    } else {
        strcpy(host, phost);
    }
}

void send_error(int client_fd, int code, const char *reason, const char *msg) {
    char buf[MAXDATASIZE];
    snprintf(buf, sizeof(buf),
             "HTTP/1.0 %d %s\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %ld\r\n\r\n"
             "%s\n", code, reason, strlen(msg) + 1, msg);
    send(client_fd, buf, strlen(buf), 0);
}