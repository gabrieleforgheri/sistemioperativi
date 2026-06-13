/* NOTE: this is lacking:
 * - a proper clean termination routine that closes opened file descriptors
 * - a signal handler for managing SIGTERM
 *
 *   it is a good idea ot fix these issues...
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/guess-race.sock"
#define MAX_RAND 10

typedef struct {
    int guess;
} GuessMsg;

static volatile sig_atomic_t got_start_signal = 0;

static void start_handler(int sig) {
    (void)sig;
    got_start_signal = 1;
}

static ssize_t write_full(int fd, const void *buf, size_t count) {
    size_t done = 0;
    const char *p = buf;

    while (done < count) {
        ssize_t n = write(fd, p + done, count - done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        done += (size_t)n;
    }
    return (ssize_t)done;
}

int main(void) {
    int sock_fd;
    struct sockaddr_un srv_addr;
    struct sigaction sa_start;
    struct sigaction sa_term;
    pid_t my_pid;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path, SOCKET_PATH, sizeof(srv_addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    memset(&sa_start, 0, sizeof(sa_start));
    sa_start.sa_handler = start_handler;
    sigemptyset(&sa_start.sa_mask);
    sigaction(SIGUSR1, &sa_start, NULL);

    srand((unsigned int)time(NULL) + (unsigned int)getpid());

    my_pid = getpid();
    printf("Client started with pid %d and connected to %s\n", (int)my_pid, SOCKET_PATH);

    if (write_full(sock_fd, &my_pid, sizeof(my_pid)) != (ssize_t)sizeof(my_pid)) {
        perror("write guess");
	exit(1);
    }

    while (1) {
        GuessMsg msg;

        pause();

        if (!got_start_signal) {
            continue;
        }
        got_start_signal = 0;

        msg.guess = (rand() % MAX_RAND) + 1;

        if (write_full(sock_fd, &msg, sizeof(msg)) != (ssize_t)sizeof(msg)) {
            perror("write guess");
            break;
        }
    }

    printf("Client %d exiting cleanly...\n", (int)getpid());
    close(sock_fd);
    return 0;
}
