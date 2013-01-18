#ifndef NETWORK_C
#define NETWORK_C

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include "logger.c"

#define PORT "3490" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define SIZE 4096 // size of messages and buffers

enum {SERVER, CLIENT};

struct addrinfo get_hints() {
   struct addrinfo hints;
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE; // use my IP
   return hints;
}

struct addrinfo *get_results(const char *node, struct addrinfo hints) {
   struct addrinfo *servinfo;
   int status;
   if ((status = getaddrinfo(node, PORT, &hints, &servinfo)) != 0) {
      log_message(LOG_FILE, "getaddrinfo: ");
      log_message(LOG_FILE, gai_strerror(status));
      exit(EXIT_FAILURE);
   }
   return servinfo;
}

int get_server_socket(const char *node) {
   struct addrinfo hints, *servinfo, *p;
   int sockfd, yes = 1;
   
   hints = get_hints();
   servinfo = get_results(node, hints);
   
   // loop through all the results and bind to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         log_error(LOG_FILE, "socket");
         continue;
      }
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         log_error(LOG_FILE, "setsockopt");
         exit(EXIT_FAILURE);
      }
      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         log_error(LOG_FILE, "bind");
         continue;
      }
      if (listen(sockfd, BACKLOG) == -1) {
         close(sockfd);
         log_error(LOG_FILE, "listen");
         continue;
      }
      break;
   }
   if (p == NULL) {
      log_message(LOG_FILE, "failed to bind\n");
      exit(EXIT_FAILURE);
   }
   freeaddrinfo(servinfo); // all done with this structure
   
   return sockfd;
}

int get_client_socket(const char *node) {
   struct addrinfo hints, *servinfo, *p;
   int sockfd, yes = 1;
   
   hints = get_hints();
   servinfo = get_results(node, hints);
   
   // loop through all the results and bind to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         perror("socket");
         continue;
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         perror("connect");
         continue;
      }
      break;
   }
   freeaddrinfo(servinfo); // all done with this structure
   if (p == NULL)
      return -1;
   else
      return sockfd;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
   if (sa->sa_family == AF_INET) // IPv4
      return &(((struct sockaddr_in*)sa)->sin_addr);
   return &(((struct sockaddr_in6*)sa)->sin6_addr); // IPv6
}

int accept_connection(int sockfd) {
   int new_fd; // listen on sockfd, new connection on playfd
   struct sockaddr_storage their_addr; // connector's address information
   socklen_t sin_size;
   char s[INET6_ADDRSTRLEN];
   
   sin_size = sizeof their_addr;
   new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
   if (new_fd == -1)
      log_error(LOG_FILE, "accept");
   else {
      inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
      log_message(LOG_FILE, "got connection from ");
      log_message(LOG_FILE, s);
      log_message(LOG_FILE, "\n");
   }
   return new_fd;
}

// sends msg to socket
// mode determines the logging method
int send_message(int socket, char *msg, int mode) {
   if (send(socket, msg, strlen(msg) + 1, MSG_CONFIRM) == -1) {
      if (mode == SERVER)
         log_error(LOG_FILE, "send"); // server does not have stderr
      else if (mode == CLIENT)
         perror("send"); // client has stderr
      return -1;
   }
   return 1;
}

int recv_message(int socket, char buf[][SIZE], int mode) {
   int n_chars = recv(socket, *buf, sizeof *buf, 0);
   if (n_chars == -1) { // error
      if (mode == SERVER)
         log_error(LOG_FILE, "recv"); // server does not have stderr
      else if (mode == CLIENT)
         perror("recv"); // client has stderr
   }
   else if (n_chars == 0) { // socket closed the connection
      if (mode == SERVER) {
         char msg[SIZE];
         sprintf(msg, "connection to socket %d closed\n", socket);
         log_message(LOG_FILE, msg);
      }
      else if (mode == CLIENT)
         printf("server closed the connection");
   }
   return n_chars;
}

#endif
