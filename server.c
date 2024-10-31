/*
 * STRUCTURA DE BAZA A CODULUI ESTE LUATA DIN LABORATORUL 7
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#include "common.h"
#include "client.h"

// sentinel for the list of clients
struct client *head;

void evaluate_communication(int tcp_socket, int udp_socket) {

    // initial capacity for the dinamically allocated poll
    int init_capacity = INIT_CAPACITY;
    struct pollfd *poll_fds = malloc(init_capacity * sizeof(struct pollfd));
    DIE(poll_fds < 0, "malloc poll");
    int size_poll = 0;

    // 3 default sockets
    int num_sockets = 3;
    int rc;

    // add the udp_socket to the poll
    poll_fds[0].fd = udp_socket;
    poll_fds[0].events = POLLIN;
    size_poll++;

    // add the listenfd socket to the poll
    poll_fds[1].fd = tcp_socket;
    poll_fds[1].events = POLLIN;
    size_poll++;

    // also add standard input to the poll
    poll_fds[2].fd = STDIN_FILENO;
    poll_fds[2].events = POLLIN;
    size_poll++;

    int stop_cond = 1;

    while (stop_cond) {
        // wait to receive something on one of the sockets
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                // standard input
                if (poll_fds[i].fd == STDIN_FILENO) {
                    char message[5];
                    if (fgets(message, sizeof(message), stdin) != NULL) {
                        if (strncmp(message, "exit", 4) == 0) {
                            // send exit message to all clients
                            send_exit_to_clients(head);

                            // exit the loop
                            stop_cond = 0;
                            break;
                        }
                    }
                } else if (poll_fds[i].fd == udp_socket) {
                    // upd message received
                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    char buffer[MSG_MAXSIZE];
                    memset(buffer, 0, MSG_MAXSIZE);
                    rc = recvfrom(udp_socket, buffer, MSG_MAXSIZE, 0,
                                 (struct sockaddr*)&cli_addr, &cli_len);
                    DIE(rc < 0, "recv");

                    // message
                    struct message_udp* message = (struct message_udp*)buffer;

                    // set ip source address and port for the message
                    message->saddr = cli_addr.sin_addr;
                    message->sport = cli_addr.sin_port;

                    // send messaged to the subscribed clients
                    send_message_to_clients(head, message);
                } else if (poll_fds[i].fd == tcp_socket) {
                    // received a connection request on the listening socket
                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    const int newsockfd = accept(tcp_socket, 
                        (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(newsockfd < 0, "accept");

                    // stop Nagle's algorithm
                    int enable = 1;
                    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, 
                        (char*)&enable, sizeof(enable));

                    // receive the message (id in this case)
                    char received[11];
                    memset(received, 0, ID_SIZE);
                    receive_data(newsockfd, received);

                    // initialize a new client with the received information
                    struct client *new_client = cli_init(received);

                    // add client to the list, check if they already exist
                    int success = addClient(head, new_client, newsockfd);

                    if (success == 0) {
                        // client has to be disconnected
                        disconnect(head, new_client, newsockfd);
                        
                        printf("Client %s already connected.\n", new_client->id);
                        free(new_client);
                        continue;
                    }

                    printf("New client %s connected from %s:%u.\n",
                        received, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);

                    // add the new socket to the server poll
                    size_poll++;

                    // reallocate memory if necessary and double the capacity
                    if (size_poll == init_capacity) {
                        init_capacity *= 2;
                        poll_fds = realloc(poll_fds, 
                            init_capacity * sizeof(struct pollfd));
                    }
                    
                    poll_fds[num_sockets].fd = newsockfd;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;
                } else {
                    // received data on one of the client sockets
                    char received[sizeof(struct message_udp)];
                    memset(received, 0, ID_SIZE);
                    receive_data(poll_fds[i].fd, received);

                    // a client has executed "exit" so the server 
                    // has to print that the client has disconnected
                    if (strncmp(received, "exit", 4) == 0) {
                        // get id for the client
                        char *id = identify_client_id(head, poll_fds[i].fd);
                        printf("Client %s disconnected.\n", id);

                        // remove client from the list
                        struct client *crt_client = identify_client(head, poll_fds[i].fd);
                        remove_client(head, crt_client);

                        // decrease the number of sockets since the client's one was closed
                        close(poll_fds[i].fd);
                        for (int j = 0; j < num_sockets; j++) {
                            if (poll_fds[j].fd == i) {
                                // shift the elements to the left by 1 position
                                for (int k = j; k < num_sockets - 1; k++) {
                                    poll_fds[k] = poll_fds[k + 1];
                                }

                                // nullify the last position occupied
                                memset(&poll_fds[num_sockets - 1], 0, sizeof(struct pollfd));
                                num_sockets--;
                                break;
                            }
                        }
                    } else if (strncmp(received, "subscribe", 9) == 0) {
                        // topic will store the argument from the subscribe command
                        char topic[TOPIC_MAXSIZE];
                        memset(topic, 0, TOPIC_MAXSIZE);
                        
                        // command is used only for reading the input but never used
                        char command[10];
                        sscanf(received, "%s %s", command, topic);
                        
                        // send message to subscriber
                        char message[SUBSCRIBE_SIZE] = "subscribed";
                        strcat(message, " ");
                        strcat(message, topic);
                        send_data(poll_fds[i].fd, message, strlen(message) + 1);

                        // add the subscription for the current client
                        struct client *crt_client = identify_client(head, poll_fds[i].fd);
                        add_topic_for_client(head, crt_client, topic);

                    } else if (strncmp(received, "unsubscribe", 11) == 0) {
                        // topic will store the argument from the unsunscribe command
                        char topic[TOPIC_MAXSIZE];
                        memset(topic, 0, TOPIC_MAXSIZE);

                        // command is used only for reading the input but never used
                        char command[10];
                        sscanf(received, "%s %s", command, topic);

                        // send message to subscriber
                        char message[UNSUBSCRIBE_SIZE] = "unsubscribed";
                        strcat(message, " ");
                        strcat(message, topic);
                        send_data(poll_fds[i].fd, message, strlen(message) + 1);

                        // remove the subscription for the current client
                        struct client *crt_client = identify_client(head, poll_fds[i].fd);
                        remove_topic_for_client(head, crt_client, topic);
                    }
                }
            }
        }
    }

    // free the poll
    free(poll_fds);

    // free the list and the list of topics associated to every client
    free_list(head);
}

int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE (argc != 2, "start error");

    // server data
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // udp socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "udp socket");

    // bind udp - server
    int rc = bind(udp_socket, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind udp");

    // tcp socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "tcp socket");

    // disable Nagle's algorithm
    int enable = 1;
    setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));

    // bind tcp - server
    rc = bind(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(rc < 0, "bind tcp");

    // set tcp_socket for listening
    rc = listen(tcp_socket, 100);
    DIE(rc < 0, "listen");

    // initialize the head of the client list
    head = init_client_list();

    evaluate_communication(tcp_socket, udp_socket);
    
    // close the sockets
    close(udp_socket);
    close(tcp_socket);

    return 0;
}