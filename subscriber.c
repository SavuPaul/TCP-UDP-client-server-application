/*
 * STRUCTURA DE BAZA A CODULUI ESTE LUATA DIN LABORATORUL 7
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * subscriber.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

void run_client(int sockfd) {
    // initial capacity for the dinamically allocated poll
    int init_capacity = INIT_CAPACITY;
    struct pollfd *poll_fds = malloc(init_capacity * sizeof(struct pollfd));
    DIE(poll_fds < 0, "malloc poll");
    int size_poll = 0;

    // there will be 2 sockets
    int num_sockets = 2;

    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    size_poll++;

    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN;
    size_poll++;

    int stop_cond = 1;

    while (stop_cond) {
        // wait to receive information on one of the two sockets
        int rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                char message[2000];
                // standard input
                if (poll_fds[i].fd == STDIN_FILENO) {
                    memset(message, 0, sizeof(message));
                    if (fgets(message, sizeof(message), stdin) != NULL) {
                        // message is "exit"
                        if (strncmp(message, "exit", 4) == 0) {
                            size_t length = strlen("exit");
                            // send "exit" message from client to server so that the server
                            // prints the "Client <ID> disconnected." message
                            send_data(sockfd, message, length + 1);

                            // close the socket
                            close(sockfd);

                            // exit while
                            stop_cond = 0;
                            break;

                        } else if (strncmp(message, "subscribe", 9) == 0) {
                            //message is "subscribe"
                            size_t length = strlen("subscribe");
                            send_data(sockfd, message, length + TOPIC_MAXSIZE + 1);

                            // receive the data and print subscribe confirmation
                            char buffer[SUBSCRIBE_SIZE];
                            receive_data(sockfd, buffer);
                            char topic[TOPIC_MAXSIZE];
                            char command[11];
                            sscanf(buffer, "%s %s", command, topic);
                            printf("Subscribed to topic %s\n", topic);

                        } else if (strncmp(message, "unsubscribe", 11) == 0) {
                            // message is "unsubscribe"
                            size_t length = strlen("unsubscribe");
                            send_data(sockfd, message, length + TOPIC_MAXSIZE + 1);

                            // receive the data and print unsubscribe confirmation
                            char buffer[UNSUBSCRIBE_SIZE];
                            receive_data(sockfd, buffer);
                            char topic[TOPIC_MAXSIZE];
                            char command[11];
                            sscanf(buffer, "%s %s", command, topic);
                            printf("Unsubscribed from topic %s\n", topic);
                        }
                    }
                // socket    
                } else if (poll_fds[i].fd == sockfd) {
                    // max dimension can be sizeof(struct message_udp)
                    char received[sizeof(struct message_udp)];

                    // receive the data
                    receive_data(sockfd, received);

                    // received command "exit" from server
                    if (strncmp(received, "exit", 4) == 0) {
                        stop_cond = 0;
                        break;
                    } else {
                        struct message_udp *message = (struct message_udp *)received;

                        // message types (0, 1, 2, 3)
                        if (message->type == 0) {
                            // sign byte
                            uint8_t sign_byte;
                            memcpy(&sign_byte, message->content, sizeof(uint8_t));

                            // number
                            uint32_t number;
                            memcpy(&number, message->content + sizeof(uint8_t), sizeof(uint32_t));

                            // network to host long
                            number = ntohl(number);

                            // prints according to the sign byte value
                            if (sign_byte == 0) {
                                printf("%s:%u - %s - INT - %d\n", 
                                        inet_ntoa(message->saddr), 
                                        message->sport, 
                                        message->topic, 
                                        number);
                            } else {
                                printf("%s:%u - %s - INT - -%d\n", 
                                        inet_ntoa(message->saddr), 
                                        message->sport, 
                                        message->topic, 
                                        number);
                            }
                        } else if (message->type == 1) {
                            // number
                            uint16_t number;
                            memcpy(&number, message->content, sizeof(uint16_t));

                            // network to host short
                            number = ntohs(number);

                            // divide number by 100 and make it positive if it's negative
                            double new_number = (double)number / 100;
                            if (new_number < 0) {
                                new_number = new_number * (-1);
                            }
                            printf("%s:%u - %s - SHORT_REAL - %.2f\n", 
                                    inet_ntoa(message->saddr), 
                                    message->sport, message->topic, 
                                    new_number);
                        } else if (message->type == 2) {
                            uint8_t sign_byte;
                            uint32_t number;
                            uint8_t module_pow;

                            memcpy(&sign_byte, message->content, sizeof(uint8_t));
                            memcpy(&number, message->content + sizeof(uint8_t),
                                    sizeof(uint32_t));
                            memcpy(&module_pow, message->content + sizeof(uint8_t) + sizeof(uint32_t),
                                sizeof(uint8_t));

                            // network to host long
                            number = ntohl(number);
                            
                            // calculate the power of 10 based on the data read earlier
                            int power = 1;
                            for (int i = 0; i < module_pow; i++) {
                                power *= 10;
                            }

                            // calculate new number and print it according to sign byte
                            double new_number = (double) number / power;
                            if (sign_byte == 1) {
                                printf("%s:%u - %s - FLOAT - -%.4f\n",
                                        inet_ntoa(message->saddr),
                                        message->sport,
                                        message->topic,
                                        new_number);
                            } else {
                                printf("%s:%u - %s - FLOAT - %.4f\n",
                                        inet_ntoa(message->saddr),
                                        message->sport,
                                        message->topic,
                                        new_number);
                            }
                        } else if (message->type == 3) {
                            // print the string
                            printf("%s:%u - %s - STRING - %s\n", 
                                    inet_ntoa(message->saddr), 
                                    message->sport, 
                                    message->topic, 
                                    message->content);
                        }
                    }
                }
            }
        }
    }

    // free the poll memory
    free(poll_fds);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE (argc != 4, "subscriber");

    // initalize server data
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    int rc = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(rc <= 0, "inet_aton");

    // TCP socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "tcp_socket");
    
    // disable Nagle's algorithm
    int enable = 1;
    setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));

    // connect
    rc = connect(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

    // send id to server
    size_t length = strlen(argv[1]);
    send_data(tcp_socket, argv[1], length + 1);

    run_client(tcp_socket);
    
    return 0;
}