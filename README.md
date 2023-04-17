# IPK Project 2 - IOTA: Server for Remote Calculator
Name and surname: Roman Poliačik\
Login: xpolia05

## Purpose

This project implements a server for the IPK Calculator Protocol(__IPKCP__). The server can communicate with any client using IPKCP, supporting both TCP and UDP modes. Server can also handle multiple clients concurrently and graceful exit upon receiving an interrupt signal (Ctrl+C).

## User Datagram Protocol (UDP)

UDP is a simple protocol for sending data between devices without establishing a connection. In this program, UDP is used for quick communication. Although data loss might happen, it results in lower latency and less overhead compared to TCP. The `handle_udp_client` function takes care of incoming UDP messages and responds as needed.

## Transmission Control Protocol (TCP)

TCP is a more complex protocol that guarantees reliable and ordered data transfer by creating connections between devices. In this program, TCP is used for client-server communications that need a stable connection and assured data delivery. The `handle_tcp_client` function deals with incoming TCP connections, handles data sent in chunks, and processes messages as they are received.

## POSIX Threads (pthreads)

Pthreads is a C library for managing multiple threads running concurrently. In this program, pthreads help improve performance and allows multiple connections by allowing multiple threads to handle TCP clients at the same time. The `main` function creates a thread for each client connection, and the `handle_tcp_client` function is used in these threads, enabling parallel processing of client requests.

## Dependencies

- `GCC Compiler`

The source code depends on the following C libraries:

- `stdio.h`: For standard input and output functions.
- `stdlib.h`: For memory allocation and process control functions.
- `string.h`: For string manipulation functions.
- `sys/socket.h`: For socket definitions.
- `sys/types.h`: For system data types.
- `netinet/in.h`: For internet address family definitions.
- `arpa/inet.h`: For definitions of internet operations.
- `unistd.h`: For standard symbolic constants and types.
- `signal.h`: For signal handling functions.
- `pthread.h`: For thread management and creation.
- `ctype.h`: For character classification and conversion functions.
- `iostream`: For standard input/output (C++).

## Build and Execution 
To build the source code, run one of the following commands:

__`g++ ipkcpd.cpp -o ipkcpd`__\
or using make:\
__`make`__

To execute the client application, run the following command:

__`./ipkcpd -h <host> -p <port> -m <mode>`__

- `-h <host>`: Specifies the IPv4 host address to listen on. (default: localhost/0.0.0.0)
- `-p <port>`: Specifies the port number to listen on. (default: 2023)
- `-m <mode>`: specifies the communication protocol (`tcp` or `udp`) (default: tcp)

## Functions

The source code consists of the following functions:

### `parameter_check(int argc, char* argv[], const char **host, const char **port, const char **mode)`
Checks and validates the command line arguments for the server. It sets default values for host, port, and mode, and processes the given arguments.

### `parse_number(char *expr, long long *result)`
Parses a number from the given expression and stores the result in the result variable. It returns a pointer to the character after the parsed number.

### `parse_operator(char *expr, char *op)`
Parses an operator from the given expression and stores it in the op variable. It returns a pointer to the character after the parsed operator.

### `parse_expr(char *expr, long long *result)`
Parses an arithmetic expression in parentheses and returns the result. It can handle nested expressions and basic arithmetic operations (+, -, *, /). It returns a pointer to the character after the parsed expression.

### `evaluate_expression(char *expression, long long *result)`
Evaluates the given arithmetic expression and stores the result in the provided result variable. It returns 0 on success and -1 on error (invalid expression or negative number).

### `sigint_handler(int sig)`
This function is a signal handler that handles `CTRL-c` events. It ensures graceful closing of connections between the server and its clients before exiting the program.

### `close_gracefully()`
Closes server socket which handles UDP graceful exit and then closes all active TCP client's connections gracefully. It's called by the sigint_handler.

### `setup_socket(int *server_socket, const char *host, const char *port, const char *mode)`
Sets up the server socket with the specified host, port, and mode (TCP or UDP). It binds the socket to the specified address and listens for incoming connections if in TCP mode.

### `add_client(int client_socket)`
Adds a client socket to the list of active clients. It's used to track TCP clients for gracefully closing connections and to handle `CTRL-c` event.

### `remove_client(int client_socket)`
This function removes a client socket from the list of active clients when a client disconnects or server terminates connection with the client.

### `handle_tcp_client(void *arg)`
Handles incoming TCP client connections, which may send data in chunks. It uses a finite state machine (FSM) to process client messages and perform calculations based on the received expressions. The function receives message from client, processes it according to the expected message format, and responds back. Upon receiving a "BYE" message or if an unexpected message is received or client disconnects during session, the connection is terminated, and the client is removed from the list of active TCP clients.

To handle chunks of data, the function behaves like this:

1. First after establishing connection with client, the function creates a buffer to store incoming data.
2. Function uses `recv` to receive data from client continually in a loop.
3. As data arrives in chunks, they are appended to the buffer. The function keeps track of the buffer's current size and checks if it doesn't exceed the allowed limit `MAX_RESPONSE_SIZE`, which is 255 bytes + 3 bytes for message coding. If the message length is exceeded, the connection terminates.
4. Inside the loop, the function searches for complete messages in the buffer. A complete message is defined by delimiter `\n` If a complete message is found, it is extracted from the buffer and processed accordingly.
5. If the extracted message is a termination message (e.g., "BYE"), the function closes the connection and removes the client from the list of active TCP clients. Otherwise, it continues processing the remaining data in the buffer and waits for new chunks to arrive.

### `handle_udp_client(int server_socket)`
Handles incoming UDP client messages. It processes client messages, evaluates the received arithmetic expressions, and sends the result back to the client. In case of an invalid or negative expression evaluation, an error message is sent to the client. This function does not require tracking client connections as the UDP protocol is connectionless, allowing for more straightforward message processing and response.

## Testing 
For the purpose of testing, an testing script was implemented. which can be executed as __./test.sh__.
The script iterates over test cases in folder tests, based on the mode selected in script source code, it sends input either through ipkcpc client program in UDP mode(used client from project 1, which passed all tests - I found it suitable to test the server), or netcat for TCP mode. As for netcat, it behaves differently from ipkcpc client, and does not output BYE after sending BYE to server, so the test cases have look a bit different. The output of tests with names of testcases can be found in files `test_logUDP.txt` and `test_logTCP.txt`.
During testing, I found out I have shortcomings in expression evaluation, as expression `(+3 4)` evaluates as valid in my server, even though it should not. If I were to test my server sooner, I could have had the time to fix it.

## BIBLIOGRAPHY

- Vladimir Vesely DemoTcp [online]. Publisher: Brno University of Technology, Faculty of Information Technology, January 30th 2023. [cit. 2023-17-04]. Available at: https://git.fit.vutbr.cz/NESFIT/IPK-Projekty/src/branch/master/Stubs/cpp/DemoTcp. 

- Vladimir Vesely DemoUdp [online]. Publisher: Brno University of Technology, Faculty of Information Technology, January 30th 2023. [cit. 2023-17-04]. Available at: https://git.fit.vutbr.cz/NESFIT/IPK-Projekty/src/branch/master/Stubs/cpp/DemoUdp.

- Vladimir Vesely DemoFork [online]. Publisher: Brno University of Technology, Faculty of Information Technology, January 30th 2023. [cit. 2023-17-04]. Available at: https://git.fit.vutbr.cz/xschie03/IPK-Projekty/src/branch/master/Stubs/cpp/DemoFork.

- C++ Multithreading [online]. Publisher: Tutorialspoint. [cit. 2023-17-04]. Available at: https://www.tutorialspoint.com/cplusplus/cpp_multithreading.htm

- pthread_mutex_init()--Initialize Mutex [online]. Publisher: IBM, April 14, 2021 [cit. 2023-17-04]. Available at: https://www.ibm.com/docs/en/i/7.3?topic=ssw_ibm_i_73/apis/users_61.html

- pthread_create - create a new thread [online]. Publisher: The Open Group, March 22, 2021 [cit. 2023-17-04]. Available at: https://man7.org/linux/man-pages/man3/pthread_create.3.html

- pthread_detach - detach a thread - create a new thread [online]. Publisher: The Open Group, March 22, 2021 [cit. 2023-17-04]. Available at: https://man7.org/linux/man-pages/man3/pthread_detach.3.html

And my first IPK project.