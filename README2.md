# README ASSIGNMENT 2 - COMMUNICATION PROTOCOLS - TCP & UDP CLIENT-SERVER APPLICATION

!!! The basic structure of the makefile and how both the server and client (subscriber) operate is taken from lab 7 !!!

The implementation consists of a server that remains open until it is forcefully stopped from stdin with the "exit" command, and clients that connect to the server via TCP sockets, allowing them to send or receive data.

Data transmission is performed with the `send_data` function, which first sends the packet size followed by the packet itself. Data reception is handled by the `receive_data` function, which first receives the packet size followed by the packet.

## SERVER IMPLEMENTATION
Clients that connect to the server and the topics to which they are subscribed are implemented as sentinel lists. I chose to implement topics as lists to make the code easier to understand (topics could also have been implemented as a character matrix).

The server is based on three primary sockets:
- One for stdin
- One for UDP
- One for TCP

In addition to these three sockets, one is added for each client that wants to connect, with communication taking place on the socket associated with that individual client's connection.

If the server receives the "exit" command from stdin, it will send the message "exit" to all clients to terminate their activities.

If the server receives data on the UDP socket, it means it has received a message, which it receives and converts into a variable of type `(struct message_udp *)` to save the IP address and port, making it easier to display messages from clients. This UDP message is then forwarded to all clients subscribed to the topic of the respective message.

If the server receives data on the TCP socket, it means a new client wants to connect. The ID is received, and if it does not already exist, the client is added to the application's client list, and its socket is added to the server's poll of sockets. If the ID already exists, the client is rejected, and the associated socket is closed.

If the server receives data from the clients' sockets, it can be of the following types:
- **exit** -> The client indicates that it is closing, and the server must print the corresponding message, remove the client from the application's client list, close the connection socket, and remove the socket from the server's poll.
- **subscribe** -> The client subscribes to a topic, which must be added to the client's topic list.
- **unsubscribe** -> The client unsubscribes from a topic, which is removed from the client's topic list.

## SUBSCRIBER IMPLEMENTATION
The client is based on two sockets:
- One TCP socket for communication with the server
- One for stdin

If the client receives data from stdin, the commands can be:
- **exit** -> The client sends a message to the server to disconnect and closes the socket. The server prints that the subscriber has disconnected.
- **subscribe** -> The client sends the topic it wishes to subscribe to and displays a confirmation message ("Subscribed to topic <TOPIC>").
- **unsubscribe** -> The client sends the topic it wishes to unsubscribe from and displays a confirmation message ("Unsubscribed from topic <TOPIC>").

If the client receives data from the socket (i.e., from the server), it can be:
- **exit** -> When the server is stopped with the "exit" command from stdin, the client must terminate its activity.
- **UDP message** -> These messages can be of various types, and the output is constructed based on the message type, as per the requirement. The source addresses and ports are displayed using the two fields in the UDP message structure.
