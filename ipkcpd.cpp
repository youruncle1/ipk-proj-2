#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

int client_socket; //client socket #
char *mode; //connection mode

void parameter_check(int argc, char* argv[],char **host,char **port,char **mode) {
    int option;

    /* Check if there is correct amount of parameters */
    if (argc != 7) {
        fprintf(stderr, "Usage: ipkcpc -h <host> -p <port> -m <mode>\n");
        exit(EXIT_FAILURE);
    }

    /* Get parameters */
    while ((option = getopt(argc, argv, "h:p:m:")) != -1) {
        switch (option) {
        case 'h':
            *host = optarg;
            break;
        case 'p':
            *port = optarg;
            break;
        case 'm':
            *mode = optarg;
            break;
        default:
            fprintf(stderr, "Usage: ipkcpc -h <host> -p <port> -m <mode>\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Check if required parameters are present */
    if (host == NULL || port == NULL || mode == NULL) {
        fprintf(stderr, "Usage: ipkcpc -h <host> -p <port> -m <mode>\n");
        exit(EXIT_FAILURE);
    }

    /* Check if HOST is a valid IPv4 address 
       https://man7.org/linux/man-pages/man3/inet_aton.3.html */
    struct in_addr addr;
    if (inet_aton(*host, &addr) == 0) {
        fprintf(stderr, "ERROR: Invalid IPv4 address: %s\n", *host);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    char *host; //host address (IPv4)
    char *port; //port #

    parameter_check(argc, argv, &host, &port, &mode);
    return 0;
}