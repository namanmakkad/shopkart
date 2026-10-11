#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
  #define INVALID_SOCK INVALID_SOCKET
  #define CLOSESOCKET closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int socket_t;
  #define INVALID_SOCK (-1)
  #define CLOSESOCKET close
#endif

#include <stddef.h>

#define MAX_HEADER_SIZE 8192
#define MAX_BODY_SIZE   16384
#define MAX_HEADERS     64
#define MAX_PATH_LEN    2048
#define MAX_QUERY_LEN   4096
#define SERVER_PORT     8080

typedef struct {
    char method[16];
    char path[MAX_PATH_LEN];
    char query_string[MAX_QUERY_LEN];
    struct {
        char name[256];
        char value[1024];
    } headers[MAX_HEADERS];
    int header_count;
    char body[MAX_BODY_SIZE + 1];
    int body_length;
    char session_token[256];
} HttpRequest;

typedef struct {
    int status_code;
    char status_text[64];
    char content_type[128];
    char *body;
    size_t body_length;
    char extra_headers[4096];
} HttpResponse;

int server_init(int port);
void server_run(void);
void server_cleanup(void);

void response_init(HttpResponse *resp);
void response_set_status(HttpResponse *resp, int code, const char *text);
void response_set_body(HttpResponse *resp, const char *body, size_t length);
void response_set_html(HttpResponse *resp, const char *html);
void response_set_redirect(HttpResponse *resp, const char *url);
void response_add_header(HttpResponse *resp, const char *header_line);
void response_free(HttpResponse *resp);

const char *request_get_header(const HttpRequest *req, const char *name);

#endif
