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
#include <ctype.h>

char *mode; //connection mode for signal handler has to be global

#define MAX_RESPONSE_SIZE 259

void sigint_handler(int sig) {
    /* TODO */
    (void) sig;
    exit(EXIT_FAILURE);
}

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

    /* Check if PORT is a valid number */
    long port_num = strtol(*port, nullptr, 10);
    if (port_num < 1 || port_num > 65535) {
        fprintf(stderr, "ERROR: Invalid port number: %s\n", *port);
        exit(EXIT_FAILURE);
    }

    /* Check if MODE is either tcp || ucp */
    if (strcmp(*mode, "tcp") != 0 && strcmp(*mode, "udp") != 0) {
        fprintf(stderr, "ERROR: mode must be either tcp or udp");
        exit(EXIT_FAILURE);
    }
}

char *parse_number(char *expr, long long *result) {
    if (*expr == '-') {
        return expr;
    }
    long long num = 0;
    while (isdigit(*expr)) {
        num = num * 10 + (*expr - '0');
        expr++; 
    }
    *result = num;
    return expr;
}

char *parse_operator(char *expr, char *op) {
    if (*expr == '+' || *expr == '-' || *expr == '*' || *expr == '/') {
        *op = *expr;
        expr++;
    } else {
        *op = '\0';
    }
    return expr;
}

char *parse_expr(char *expr, long long *result) {
    if (isdigit(*expr)) {
        return parse_number(expr, result);
    }

    if (*expr == '(') {
        expr++;
        char op;
        expr = parse_operator(expr, &op);
        if (op == '\0') {
            return expr;
        }

        expr++; // Skip space after operator
        long long first_operand;
        expr = parse_expr(expr, &first_operand);
        *result = first_operand;

        while (*expr == ' ') {
            long long operand;
            expr = parse_expr(++expr, &operand);

            switch (op) {
                case '+':
                    *result += operand;
                    break;
                case '-':
                    *result -= operand;
                    break;
                case '*':
                    *result *= operand;
                    break;
                case '/':
                    if (operand == 0) {
                        return expr;
                    }
                    *result /= operand;
                    break;
            }
        }

        if (*expr == ')') {
            expr++;
        }
    }

    return expr;
}

long long evaluate_expression(char *expression, long long *result) {
    expression = parse_expr(expression, result);
    if (*expression == '\0' || *expression == '\n') {
        return 0;
    } else {
        return -1;
    }
}

int main(int argc, char *argv[]) {
    char *host; //host address (IPv4)
    char *port; //port #

    /* Parse command line arguments and initialize variables. */
    parameter_check(argc, argv, &host, &port, &mode);

    /* Create server socket based on mode */
    int server_socket;
    if (strcmp(mode, "tcp") == 0) {
        /* Create TCP socket */
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
            perror("ERROR: socket");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(mode, "udp") == 0) {
        /* Create UDP socket */
        if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
            perror("ERROR: socket");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Invalid mode: %s\n", mode);
        exit(EXIT_FAILURE);
    }

    /* Bind the server socket to the specified address and port. */
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(host);
    server_address.sin_port = htons(std::atoi(port));

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }

    /* Prepare signal handler */
    signal(SIGINT, sigint_handler);

    while (1) {
        if (strcmp(mode, "tcp") == 0) {
            /* TODO */
            return 0;
        
        } else if (strcmp(mode, "udp") == 0) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            unsigned char response_buffer[MAX_RESPONSE_SIZE];
            size_t response_length;
            const char *error_message = "Could not parse message";

            ssize_t recv_len = recvfrom(server_socket, response_buffer, sizeof(response_buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);

            if (recv_len < 0) {
                perror("recvfrom() failed");
                continue;
            }

            if (recv_len < 3 || response_buffer[0] != 0x00 || recv_len - 2 != response_buffer[1]) {
                /* Wrong message format / Bad Opcode / Payload length wrong */
                response_buffer[0] = 0x01; // Opcode: response
                response_buffer[1] = 0x01; // Status code: error  
                response_buffer[2] = strlen(error_message);
                memcpy(&response_buffer[3], error_message, response_buffer[2]);
                response_length = 3 + response_buffer[2];
            } else {
                long long result;
                if (evaluate_expression((char *)&response_buffer[2], &result) == 0 && result >= 0) {
                    response_buffer[0] = 0x01; // Opcode: response
                    response_buffer[1] = 0x00; // Status code: error
                    //https://forum.arduino.cc/t/issues-creating-a-library-with-char-arrays/853215
                    response_length = snprintf((char *)&response_buffer[3], sizeof(response_buffer) - 3, "%lld", result);
                    response_buffer[2] = response_length;
                    response_length += 3;
                } else {
                    /* Expression invalid */
                    response_buffer[0] = 0x01; // Opcode: response
                    response_buffer[1] = 0x01; // Status code: error
                    response_buffer[2] = strlen(error_message);
                    memcpy(&response_buffer[3], error_message, response_buffer[2]);
                    response_length = 3 + response_buffer[2];
                }
            }
            
            /* Send the message to client */
            sendto(server_socket, response_buffer, response_length, 0, (struct sockaddr *)&client_addr, client_addr_len);

        }
    }
    /* Close the server socket and exit (should not happen now anyways) */
    close(server_socket);
    exit(EXIT_SUCCESS);
}

