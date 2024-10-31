#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1551
#define ID_SIZE 11
#define INIT_CAPACITY 100
#define TOPIC_MAXSIZE 51
#define MAX_TOPICS 100
#define SUBSCRIBE_SIZE 62
#define UNSUBSCRIBE_SIZE 64

// taken from lab
#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

void send_data(int socket, const void* data, size_t length);
void receive_data(int socket, void* buff);

// structure for the udp message
struct message_udp {
  char topic[50];
  uint8_t type;
  char content[1500];
  struct in_addr saddr;
	in_port_t sport;
};

#endif
