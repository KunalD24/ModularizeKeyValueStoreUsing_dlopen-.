#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#define SOCKET_PATH "/tmp/keyvaluestore.sock"
#define BUF_SIZE 1024
#define MAX_KEY 256
#define MAX_VALUE 512

static int listen_fd = -1;

void die(const char *msg) 
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void cleanup(void) 
{
    if (listen_fd != -1) 
    {
        close(listen_fd);
    }
    unlink(SOCKET_PATH);
}

void on_signal(int sig) 
{
    (void)sig;
    exit(0);
}

ssize_t read_line(int fd, char *buf, size_t maxlen) 
{
    size_t n = 0;
    while (n + 1 < maxlen) 
    {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r <= 0) break;
        buf[n++] = c;
        if (c == '\n') break;
    }
    buf[n] = '\0';
    return n;
}

int write_all(int fd, const void *buf, size_t len) 
{
    const char *p = buf;
    while (len > 0) 
    {
        ssize_t w = write(fd, p, len);
        if (w < 0) return -1;
        p += w;
        len -= w;
    }
    return 0;
}

int main(void) 
{
    atexit(cleanup);
    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    void *handle;
    char *(*get_value)(const char *);
    void (*set_value)(const char *, const char *);
    char *error;

    handle = dlopen("./libkvstore.so", RTLD_LAZY);
    if (!handle) 
    {
        fprintf(stderr, "Error loading library: %s\n", dlerror());
        return 1;
    }

    get_value = dlsym(handle, "get_value");
    if ((error = dlerror()) != NULL) 
    {
        fprintf(stderr, "Error finding get_value(): %s\n", error);
        return 1;
    }

    set_value = dlsym(handle, "set_value");
    if ((error = dlerror()) != NULL) 
    {
        fprintf(stderr, "Error finding set_value(): %s\n", error);
        return 1;
    }

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) die("socket");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    unlink(SOCKET_PATH);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) die("bind");
    if (listen(listen_fd, 5) == -1) die("listen");

    printf("KV server listening on %s\n", SOCKET_PATH);

    while (1) 
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd == -1) continue;

        while (1) 
        {
            char buf[BUF_SIZE];
            ssize_t n = read_line(client_fd, buf, sizeof(buf));
            if (n <= 0) break; 

            char key[MAX_KEY], value[MAX_VALUE];
            if (sscanf(buf, "SET %255s %767[^\n]", key, value) == 2) 
            {
                set_value(key, value);
                write_all(client_fd, "OK\n", 3);
            } 
            else if (sscanf(buf, "GET %255s", key) == 1) 
            {
                char *val = get_value(key);
                if (val) 
                {
                    char out[BUF_SIZE];
                    snprintf(out, sizeof(out), "%s\n", val);
                    write_all(client_fd, out, strlen(out));
                } 
                else 
                {
                    write_all(client_fd, "NOTFOUND\n", 9);
                }
            } 
            else 
            {
                write_all(client_fd, "ERROR\n", 6);
            }
        }
        close(client_fd);
    }

    dlclose(handle);
    return 0;
}

// gcc -fPIC -shared -o libkvstore.so kv_store_dynamic.c
// gcc server.c -ldl -o kv_server