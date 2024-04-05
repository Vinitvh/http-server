#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define IP "127.0.0.1"

// Http Request structure
typedef struct HttpRequest {
  char method[8];
  char url[256];
} httpreq;

/*  Socket Initialization with port. Returns -1 on error or it returns a socket
 * fd */
int socket_init(int port) {
  int s;
  struct sockaddr_in server_addr;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(IP);
  server_addr.sin_port = htons(port);

  if (bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Socket binding error");
    exit(EXIT_FAILURE);
  }

  if (listen(s, 5) < 0) {
    perror("Socket listening error");
    close(s);
    exit(EXIT_FAILURE);
  }

  return s;
}

int client_accept(int fd) {
  struct sockaddr_in client_addr;
  socklen_t addrlen;
  addrlen = 0;

  int c = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
  if (c < 0) {
    perror("Error in accepting");
    close(c);
    exit(EXIT_FAILURE);
  }

  return c;
}

httpreq *parse_http(char *str) {
  char *method = "";
  char *url = "";
  char *token;
  httpreq *req;
  
  req = malloc(sizeof(httpreq));
  if (req == NULL) {
    fprintf(stderr, "failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  // Extract method and Url from header
  char *client_header = strtok(str, "\n");
  int count = 0;
  token = strtok(client_header, " ");

  while (token != NULL) {
    switch (count) {
    case 0:
      method = token;
    case 1:
      url = token;
    }
    token = strtok(NULL, " ");
    count++;
  }

  strncpy(req->method, method, 7);
  strncpy(req->url, url, 255);
  return req;
}

void client_connect(int s, int c) {
  char buf[512];
  read(c, buf, 511);
  httpreq *p = parse_http(buf);
  printf("%s\n", p->method);
  printf("%s\n", p->url);
}

int main(int argc, char **argv) {
  int socket_fd, client_fd;
  char *port;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <listening port>", argv[0]);
    exit(EXIT_FAILURE);
  } else {
    port = argv[1];
  }

  socket_fd = socket_init(atoi(port));

  printf("Listening on port: %s/%s\n", IP, port);

  while (1) {
    client_fd = client_accept(socket_fd);
    if (!client_fd)
      return -1;
    printf("Incoming connection\n");
    if (!fork()) {
      client_connect(socket_fd, client_fd);
    }
  }
}
