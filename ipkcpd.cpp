/*
    IPK Project 2 - IOTA: Server for Remote Calculator
    author: Roman Poliacik
    login: xpolia05
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include <iostream>

#define MAX_RESPONSE_SIZE 258 // maximum size of response message from client
#define MAX_CLIENTS 128 // maximum number of simultenaously connected clients through TCP

int server_socket; // server socket for signal handler has to be global
int clients[MAX_CLIENTS]; // list of active TCP client sockets
int num_clients = 0; // number of active TCP client sockets
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex to protect access to the clients list

void parameter_check(int argc, char* argv[], const char **host, const char **port, const char **mode);
char *parse_number(char *expr, long long *result);
char *parse_operator(char *expr, char *op);
char *parse_expr(char *expr, long long *result);
long long evaluate_expression(char *expression, long long *result);
void sigint_handler(int sig);
void close_gracefully();
int setup_socket(int *server_socket, const char *host, const char *port, const char *mode);
void add_client(int client_socket);
void remove_client(int client_socket);
void *handle_tcp_client(void *arg);
void handle_udp_client(int server_socket);

void parameter_check(int argc, char* argv[], const char **host, const char **port, const char **mode) {
    int option;

    /* default values*/
    *host = "0.0.0.0";
    *port = "2023";
    *mode = "tcp";

    while ((option = getopt(argc, argv, "h:p:m:e")) != -1) {
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
        case 'e':
            std::cerr   << "Usage:\n"
                        << "  ipkcpc [-h <host>] [-p <port>] [-m <mode>]\n"
                        << "Parameters:\n"
                        << "   --help            display usage information and exit\n"
                        << "   -h <host>         host IPv4 address to listen on [default: 0.0.0.0]\n"
                        << "   -m <mode>         server mode to use [default: tcp] [modes: tcp, udp]\n"
                        << "   -p <port>         port number to listen on [default: 2023]\n";
                        exit(EXIT_SUCCESS);
        default:
            std::cerr << "run './ipkcpd -e' to show help\n";
            exit(EXIT_FAILURE);
        }
    }

    /* Check if required parameters are present */
    if (host == NULL || port == NULL || mode == NULL) {
        std::cerr << "Usage: ipkcpc -h <host> -p <port> -m <mode>\n";
        exit(EXIT_FAILURE);
    }

    /* Check if HOST is a valid IPv4 address 
       https://man7.org/linux/man-pages/man3/inet_aton.3.html */
    struct in_addr addr;
    if (inet_aton(*host, &addr) == 0) {
        std::cerr << "ERROR: Invalid IPv4 address: " << *host << "\n";
        exit(EXIT_FAILURE);
    }

    /* Check if PORT is a valid number */
    long port_num = strtol(*port, nullptr, 10);
    if (port_num < 1 || port_num > 65535) {
        std::cerr << "ERROR: Invalid port number: "<< *port << "\n";
        exit(EXIT_FAILURE);
    }

    /* Check if MODE is either tcp || ucp */
    if (strcmp(*mode, "tcp") != 0 && strcmp(*mode, "udp") != 0) {
        std::cerr << "ERROR: mode must be either tcp or udp\n";
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

void sigint_handler(int sig) {
    (void) sig;

    close_gracefully();
}

void close_gracefully(){
    /* Close server socket (gracefully handles udp) */
    close(server_socket);

    /* Close connections with all active TCP clients (gracefully handles tcp) 
       if the mode is udp, the num_clients is 0... 
       mutex to ensure prevention of race conditions(see IOS)
       (ensures no other thread can modify clients during this event)*/
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        send(clients[i], "BYE\n", 4, 0);
        close(clients[i]);
    }
    pthread_mutex_unlock(&clients_mutex);

    exit(EXIT_SUCCESS);
}

int setup_socket(int *server_socket, const char *host, const char *port, const char *mode){
    if (strcmp(mode, "tcp") == 0) {
        /* Create TCP socket */
        if ((*server_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
            perror("ERROR: socket");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(mode, "udp") == 0) {
        /* Create UDP socket */
        if ((*server_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
            perror("ERROR: socket");
            exit(EXIT_FAILURE);
        }
    }

    /* Bind the server socket to the specified address and port. */
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(host);
    server_address.sin_port = htons(std::atoi(port));

    if (bind(*server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("ERROR: bind");
        close(*server_socket);
        return EXIT_FAILURE;
    }

    /* Listen for incoming connections if in TCP mode. */
    if (strcmp(mode, "tcp") == 0) {
        if (listen(*server_socket, 5) < 0) {
            perror("ERROR: listen() failed");
            close(*server_socket);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

void add_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    if (num_clients < MAX_CLIENTS) {
        clients[num_clients++] = client_socket;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client_socket) {
            clients[i] = clients[--num_clients];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Function to handle incoming TCP client connections. */
void *handle_tcp_client(void *arg) {
    int communications_socket = *((int *)arg);
    char response_buffer[MAX_RESPONSE_SIZE];
    int res_len;

    /* Add the client to a client list */
    add_client(communications_socket);

    /* Finite state machine (FSM) for handling TCP communication */
    enum TCP_FSM {
        AWAIT_HELLO,
        AWAIT_SOLVE,
        TERMINATE
    } state = AWAIT_HELLO; // starting state

    int buffer_len = 0;

    while (state != TERMINATE) {
        /* Receive data and append it to the existing data in the buffer */
        res_len = recv(communications_socket, response_buffer + buffer_len, MAX_RESPONSE_SIZE - buffer_len, 0);

        /* If the received length is less than or equal to 0, terminate the connection
           handles case when client closed connection without sending BYE */
        if (res_len <= 0) {
            state = TERMINATE;
            break;
        }

        buffer_len += res_len; // add length of new received data
        response_buffer[buffer_len] = '\0'; // append null terminator at end of buffer

        /* Look for the first newline character in the buffer, NULL if not present 
           if not present, the server wont go into FSM and goes back to receive new data*/
        char *newline_pos = strchr(response_buffer, '\n'); 
        
        while (newline_pos != NULL) {
            *newline_pos = '\0'; // replace \n with null terminator
            int message_len = newline_pos - response_buffer + 1; // message length(until \n)

            switch (state) {
                case AWAIT_HELLO:
                    if (strcmp(response_buffer, "HELLO") == 0) {
                        send(communications_socket, "HELLO\n", 6, 0);
                        state = AWAIT_SOLVE;
                    } else {
                        state = TERMINATE;
                    }
                    break;
                case AWAIT_SOLVE:
                    if (strncmp(response_buffer, "SOLVE ", 6) == 0) {
                        long long result;
                        if (evaluate_expression(&response_buffer[6], &result) == 0 && result >= 0) {
                            char result_str[MAX_RESPONSE_SIZE];
                            snprintf(result_str, sizeof(result_str), "RESULT %lld\n", result);
                            send(communications_socket, result_str, strlen(result_str), 0);
                        } else {
                            send(communications_socket, "BYE\n", 4, 0);
                            state = TERMINATE;
                        }
                    } else if (strcmp(response_buffer, "BYE") == 0) {
                        state = TERMINATE;
                    } else {
                        state = TERMINATE;
                    }
                    break;
                default:
                    state = TERMINATE;
            }

            if (state == TERMINATE) {
                break;
            }
            /* Remove parsed data and move the remaining data in the buffer to the beginning */
            memmove(response_buffer, newline_pos + 1, buffer_len - message_len);
            buffer_len -= message_len;
            newline_pos = strchr(response_buffer, '\n'); // look if there is another newline present
        }
        /* Check if received message is larger than aximum supported response size */
        if (buffer_len >= MAX_RESPONSE_SIZE) {
            state = TERMINATE;
        }
    }

    send(communications_socket, "BYE\n", 4, 0);
    close(communications_socket);
    remove_client(communications_socket); // if successfully terminated, remove client from list of active TCP clients
    pthread_exit(NULL);
}

/* Function to handle incoming UDP client messages. */
void handle_udp_client(int server_socket) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    unsigned char response_buffer[MAX_RESPONSE_SIZE];
    size_t response_length;
    const char *error_message = "Could not parse message";
    const char *negative_result_msg = "Server does not support negative expression evaluations";

    ssize_t recv_len = recvfrom(server_socket, response_buffer, sizeof(response_buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_len);

    if (recv_len < 0) {
        perror("recvfrom() failed");
        return;
    }

    if (recv_len < 4 || response_buffer[0] != 0x00 || recv_len - 2 != response_buffer[1]) {
        /* Missing message codes or empty msg / Bad Opcode / Payload length wrong */
        response_buffer[0] = 0x01; // Opcode: response
        response_buffer[1] = 0x01; // Status code: error
        response_buffer[2] = strlen(error_message);
        memcpy(&response_buffer[3], error_message, response_buffer[2]);
        response_length = 3 + response_buffer[2];
    } else {
        long long result;
        if (evaluate_expression((char *)&response_buffer[2], &result) == 0 && result >= 0) {
            response_buffer[0] = 0x01; // Opcode: response
            response_buffer[1] = 0x00; // Status code: OK
            response_length = snprintf((char *)&response_buffer[3], sizeof(response_buffer) - 3, "%lld", result);
            response_buffer[2] = response_length;
            response_length += 3;
        } else {
            /* Expression invalid */
            response_buffer[0] = 0x01; // Opcode: response
            response_buffer[1] = 0x01; // Status code: error
            if (result < 0){
                response_buffer[2] = strlen(negative_result_msg);
                memcpy(&response_buffer[3], negative_result_msg, response_buffer[2]);
            } else {
                response_buffer[2] = strlen(error_message);
                memcpy(&response_buffer[3], error_message, response_buffer[2]);
            }
            response_length = 3 + response_buffer[2];
        }
    }

    /* Send the message to client */
    sendto(server_socket, response_buffer, response_length, 0, (struct sockaddr *)&client_addr, client_addr_len);
}

int main(int argc, char *argv[]) {
    const char *host; //host address (IPv4)
    const char *port; //port #
    const char *mode; //connection mode 

    /* Parse command line arguments and initialize host, port and mode. */
    parameter_check(argc, argv, &host, &port, &mode);

    /* Create server socket based on mode */
    int server_socket = -1;
    setup_socket(&server_socket, host, port, mode);

    /* Prepare signal handler */
    signal(SIGINT, sigint_handler);

    while (1) {
        if (strcmp(mode, "tcp") == 0) {
            struct sockaddr_in6 client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int communications_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

            /* If accepting new connection fails */
            if (communications_socket < 0) {
                perror("accept() failed");
                continue;
            }

            /*  Create a new thread using pthread to handle new tcp connection */
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, handle_tcp_client, &communications_socket) != 0) {
                /* if creating new thread fails, close the communication and move on with life */
                perror("pthread_create() failed");
                close(communications_socket);
                continue;
            }

            /* detach allows resources to be reclaimed by system when thread exits(stops execution), 
                and also benefits the server program by running the thread independently
                (program does not need to wait for return value of thread to continue)             */
            if (pthread_detach(thread_id) != 0) {
                perror("pthread_detach() failed");
                close(communications_socket);
                continue;
            }
        } else if (strcmp(mode, "udp") == 0) {
            handle_udp_client(server_socket);
        }
    }
}
