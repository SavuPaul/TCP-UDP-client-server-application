#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

void receive_data(int socket, void* buff) {
    int rc;
    size_t bytes;
    size_t data = 0;

    // first, receive the size
    while (data != sizeof(size_t)) {
        rc = recv(socket, &bytes + data, sizeof(size_t) - data, 0);
        DIE(rc < 0, "receive_data");
        data += rc;
    }
    
    // receive the actual data
    data = 0;
    while (data < bytes) {
        rc = recv(socket, buff + data, bytes - data, 0);
        DIE(rc < 0, "receive_data");
        data += rc;
    }
}

void send_data(int socket, const void* data, size_t length) {
    int rc;

    // first, send the size
    rc = send(socket, &length, sizeof(size_t), 0);
    DIE(rc < 0, "send_data");

    // send the actual data
    rc = send(socket, data, length, 0);
    DIE(rc < 0, "send_data");
}
