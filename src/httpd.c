#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define IP "127.0.0.1"

// Http Request structure
typedef struct HttpRequest {
  char method[8];
  char url[256];
} httpreq;

// File structure
typedef struct File {
  char *buf;
  int size;
} sfile;

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

void http_header(int c, int status) {
  char buf[512];
  int n;
  snprintf(buf, 511,
           "HTTP/1.x %d OK\n"
           "Server: httpd.c\n"
           "Cache-Cantrol: no-store, no-cache, max-age=0, private\n"
           "Content-Language: en\n"
           "X-Frame-Options: SAMEORIGIN\n",
           status);
  n = strlen(buf);
  write(c, buf, n);
}

void http_response(int c, char *contentType, char *res) {
  char buf[512];
  int n;
  n = strlen(res);
  snprintf(buf, 511,
           "Content-Type: %s\n"
           "Content-Length: %d\n"
           "\n%s\n",
           contentType, n, res);
  n = strlen(buf);
  write(c, buf, n);

  return;
}

char *readFile(int c) {
  char *buf;
  FILE *file = fopen("index.html", "r");
  if (!file) {
    const char res[] = "HTTP/1.1 404 Not Found\n";
    send(c, res, sizeof(res), 0);
  } else {
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    buf = (char *)malloc(size);
    int i = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF) {
      buf[i] = ch;
      i++;
    }
  }
  fclose(file);
  return buf;
}

void client_connect(int s, int c) {
  char buf[512];
  char *res;
  read(c, buf, 511);
  httpreq *p = parse_http(buf);
  if (!p) {
    perror("Error in parsing http req");
    close(c);
    free(p);
    exit(EXIT_FAILURE);
  }

  printf("%s\n", p->method);
  printf("%s\n", p->url);

  if (!strcmp(p->method, "GET") && !strncmp(p->url, "/index.html", 11)) {
    char *res_data = readFile(c);
    http_header(c, 200);
    http_response(c, "text/html", res_data);
  } else {
    res = "File not found";
    http_header(c, 400);
    http_response(c, "text/plain", res);
  }

  if (!strcmp(p->method, "GET") && !strcmp(p->url, "/app/webpage")) {
    res = "<html>Hello World!</html>";
    http_header(c, 200);
    http_response(c, "text/html", res);
  } else {
    res = "File not found";
    http_header(c, 400);
    http_response(c, "text/plain", res);
  }

  free(p);
  close(c);
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
