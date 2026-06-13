#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/server-socket"
#define FIFO_PATH   "/tmp/client-fifo"

struct reply {
    int client_number;
    char fifo_name[108];
};

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    unlink(SOCKET_PATH);
    unlink(FIFO_PATH);

    if (mkfifo(FIFO_PATH, 0666) == -1)
        die("mkfifo");

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("bind");

    if (listen(server_fd, 2) == -1)
        die("listen");

    printf("Server in ascolto su %s\n", SOCKET_PATH);

    for (int i = 1; i <= 2; i++) {
        pid_t pid;
        struct reply r;

        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1)
            die("accept");

        if (read(client_fd, &pid, sizeof(pid)) != sizeof(pid))
            die("read pid");

        printf("Client %d con pid %d connesso\n", i, pid);

        r.client_number = i;
        strncpy(r.fifo_name, FIFO_PATH, sizeof(r.fifo_name) - 1);
        r.fifo_name[sizeof(r.fifo_name) - 1] = '\0';

        if (write(client_fd, &r, sizeof(r)) != sizeof(r))
            die("write reply");

        close(client_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);

    printf("Server terminato\n");
    return 0;
}
