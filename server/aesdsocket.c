/*
b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail. ->ok 

     c. Listens for and accepts a connection -> ok

     d. Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. -> ok

     e. Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.

Your implementation should use a newline to separate data packets received.  In other words a packet is considered complete when a newline character is found in the input receive stream, and each newline should result in an append to the /var/tmp/aesdsocketdata file.

You may assume the data stream does not include null characters (therefore can be processed using string handling functions).

You may assume the length of the packet will be shorter than the available heap size.  In other words, as long as you handle malloc() associated failures with error messages you may discard associated over-length packets.

     f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.

You may assume the total size of all packets sent (and therefore size of /var/tmp/aesdsocketdata) will be less than the size of the root filesystem, however you may not assume this total size of all packets sent will be less than the size of the available RAM for the process heap.

     g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.

     h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received (see below).

     i. Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.

Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT "9000"  // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold
#define DATA_FILE "/var/tmp/aesdsocketdata" // file to which we will append received data and read back to clients

static volatile sig_atomic_t exit_requested = 0;

void signal_handler(int signum)
{
    (void)signum; // quiet unused variable warning
    syslog(LOG_INFO, "Caught signal, exiting");
    exit_requested = 1;
}

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


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
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    struct addrinfo *servinfo;
    int rv; //return value from getaddrinfo

    // fill servinfo with address info for this host and port by using getaddrinfo()
    // !do not forget to call freeaddrinfo() when you are done with the struct addrinfo
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    struct addrinfo *p;
    int sockfd; // listen on sock_fd
    int yes=1;

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            return -1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    /* check for daemon flag (-d) */
    int daemon_mode = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            daemon_mode = 1;
            break;
        }
    }

    if (daemon_mode) {
        /* fork after successful bind/listen so parent can error out early */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }
        if (pid > 0) {
            /* parent exits */
            exit(0);
        }

        /* child becomes session leader */
        if (setsid() < 0) {
            perror("setsid");
            return -1;
        }

        /* second fork to ensure the process can't acquire a controlling terminal */
        pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        }
        if (pid > 0) exit(0);

        umask(0);
        if (chdir("/") < 0) {
            /* non-fatal */
        }

        /* redirect standard file descriptors to /dev/null */
        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2) close(fd);
        }
    }

    // setup sigactions

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    /* handle SIGINT and SIGTERM to allow graceful shutdown */
    struct sigaction sa_term;
    sa_term.sa_handler = signal_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0; /* do not set SA_RESTART so accept() is interrupted */
    if (sigaction(SIGINT, &sa_term, NULL) == -1) {
        perror("sigaction SIGINT");
        return -1;
    }
    if (sigaction(SIGTERM, &sa_term, NULL) == -1) {
        perror("sigaction SIGTERM");
        return -1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    printf("server: waiting for connections...\n");

    int new_fd; // new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address info
    socklen_t sin_size;

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
            &sin_size);
        if (new_fd == -1) {
            if (exit_requested)
                break;
            perror("accept");
            continue;
        }

        if (exit_requested) {
            close(new_fd);
            break;
        }

        char s[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);

        syslog(LOG_INFO, "Accepted connection from %s", s);

        

        if (!fork()) { // fork a child process to handle this new connection
            close(sockfd); // child doesn't need the listener

            ssize_t numbytes;
            char buf[1024];
            char *acc = NULL;
            size_t acc_len = 0;

            while ((numbytes = recv(new_fd, buf, sizeof(buf), 0)) > 0) {
                char *newacc = realloc(acc, acc_len + (size_t)numbytes);
                if (!newacc) {
                    free(acc);
                    perror("realloc");
                    break;
                }
                acc = newacc;
                memcpy(acc + acc_len, buf, (size_t)numbytes);
                acc_len += (size_t)numbytes;

                /* Process complete packets terminated by newline */
                char *nl;
                while ((nl = memchr(acc, '\n', acc_len)) != NULL) {
                    size_t packet_size = (size_t)(nl - acc) + 1; /* include newline */

                    /* Append packet to file, creating if necessary */
                    FILE *af = fopen(DATA_FILE, "a");
                    if (af) {
                        size_t w = fwrite(acc, 1, packet_size, af);
                        if (w != packet_size) {
                            perror("fwrite");
                        }
                        fflush(af);
                        fclose(af);
                    } else {
                        perror("fopen append");
                    }

                    /* Send full file contents back to client */
                    FILE *rf = fopen(DATA_FILE, "r");
                    if (rf) {
                        if (fseek(rf, 0, SEEK_END) == 0) {
                            long fsz = ftell(rf);
                            if (fsz > 0) {
                                rewind(rf);
                                char *fbuf = malloc((size_t)fsz);
                                if (fbuf) {
                                    size_t rr = fread(fbuf, 1, (size_t)fsz, rf);
                                    if (rr > 0) {
                                        ssize_t sent = send(new_fd, fbuf, rr, 0);
                                        if (sent == -1) perror("send file");
                                    }
                                    free(fbuf);
                                }
                            }
                        }
                        fclose(rf);
                    }

                    /* remove processed packet from accumulator */
                    size_t remaining = acc_len - packet_size;
                    if (remaining > 0)
                        memmove(acc, acc + packet_size, remaining);
                    acc_len = remaining;
                    if (acc_len == 0) {
                        free(acc);
                        acc = NULL;
                        break;
                    } else {
                        char *sh = realloc(acc, acc_len);
                        if (sh) acc = sh;
                    }
                }
            }

            if (numbytes == -1) perror("recv");

            free(acc);
            close(new_fd);
            syslog(LOG_INFO, "Closed connection from %s", s);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    /* cleanup on exit */
    close(sockfd);
    unlink(DATA_FILE);
    closelog();
    return 0;
}