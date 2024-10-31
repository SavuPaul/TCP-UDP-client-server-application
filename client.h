#include "common.h"

// topic structure
struct topic {
    char name[51];
    struct topic *next;
};

// client structure
struct client {
  char *id;
  int status; // socket
  int no_of_topics; // number of topics
  struct topic* topics; // list of subscribed topics
  struct client *next;
};

/* this function is used only for debugging (it is not used anywhere)
 *
 * it prints the list of clients and the topics to verify whether or not
 * adding a client, subscribe and unsubscribe are all working */
void print_list(struct client *head) {
    struct client *iter = head;

    while (iter != NULL) {
        printf("%p: ", iter);
        struct topic *iter_topic = iter->topics;
        while (iter_topic != NULL) {
            printf("%s ", iter_topic->name);
            iter_topic = iter_topic->next;
        }
        iter = iter->next;
    }
    printf("\n");
}

// initialize the sentinel for the topic list for each client
struct topic *init_topic_list() {
    struct topic *head = malloc(sizeof(struct topic));

    strcpy(head->name, "");
    head->next = NULL;

    return head;
}

// initialize a new topic with a given name
struct topic *topic_init(char *name) {
    struct topic *newTopic = malloc(sizeof(struct topic));

    strcpy(newTopic->name, name);
    newTopic->next = NULL;

    return newTopic;
}

// insert a new topic at the beginning of the topic list
void insert_topic_for_client(struct client *crt_client, char *name) {
    struct topic *new_topic = topic_init(name);

    if (crt_client->topics->next != NULL) {
        new_topic->next = crt_client->topics->next;
        crt_client->topics->next = new_topic;
    } else {
        crt_client->topics->next = new_topic;
    }
}

// initialise the sentinel for the client list
struct client *init_client_list() {
    struct client *head = malloc(sizeof(struct client));

    head->id = NULL;
    head->status = -1;
    head->no_of_topics = -1;
    head->topics = NULL;
    head->next = NULL;

    return head;
}

// initialize a new client in the list
struct client *cli_init(char *id) {
    struct client *newClient = malloc(sizeof(struct client));
    DIE(newClient == 0, "malloc client");

    newClient->id = strdup(id);
    newClient->status = 0;
    newClient->no_of_topics = 0;
    newClient->topics = init_topic_list();
    newClient->next = NULL;

    return newClient;
}

// adds client to the list
void addClientHelper(struct client *head, struct client *new_client) {
    // list is empty
    if (head->next == NULL) {
        head->next = new_client;
        return;
    }

    // list is not empty
    new_client->next = head->next;
    head->next = new_client;
}

// check if client should be added and adds them
int addClient(struct client *head, struct client *new_client, int status) {
    struct client *iter = head;

    // iterate through clients
    while (iter != NULL) {
        // check if client exists
        if (iter != head && strcmp(iter->id, new_client->id) == 0) {
            // new_client is not freed here because their id is still needed
            return 0;
        }
        iter = iter->next;
    }
    // if client does not exist, create and add them
    new_client->status = status;
    addClientHelper(head, new_client);
    return 1;
}

// removes a client from the list and deallocates memory
void remove_client(struct client *head, struct client *client_removed) {
    struct client *iter = head;

    while (iter->next != NULL) {
        if (iter->next == client_removed) {
            struct client *temp = iter->next;
            iter->next = iter->next->next;
            free(temp);
        } else {
            iter = iter->next;
        }
    }
}

// disconnects the client, closes the socket
void disconnect(struct client *head, 
    struct client *client_disconnect, int socket) {
    char message[5] = "exit";
    // send exit message
    send_data(socket, message, strlen(message) + 1);

    // close the socket
    close(socket);
}

// sends "exit" message to all existing clients when server is shut down
void send_exit_to_clients(struct client *head) {
    struct client *iter = head->next;
    while (iter != NULL) {
        if (iter->status != 0) {
            char *message = "exit";
            size_t length = strlen("exit");
            send(iter->status, &length, sizeof(size_t), 0);
            send(iter->status, message, strlen(message), 0);
        }
        iter = iter->next;
    }
}

// gets the id of an existing client whose socket is working
// on the file descriptor fd
char *identify_client_id(struct client *head, int fd) {
    struct client* iter = head;

    while (iter != NULL) {
        if (iter->status == fd) {
            return iter->id;
        }
        iter = iter->next;
    }

    return NULL;
}

// gets the client whose socket is working on the file descriptor fd
struct client *identify_client(struct client *head, int fd) {
    struct client *iter = head;

    while (iter != NULL) {
        if (iter->status == fd) {
            return iter;
        }
        iter = iter->next;
    }

    return NULL;
}

// sends the udp message only to the clients who are subscribed to the
// topic of the message
void send_message_to_clients(struct client *head, struct message_udp *message) {
    struct client *iter = head;
    
    while (iter != NULL) {
        struct topic *crt_topic = iter->topics;
        while (crt_topic != NULL) {
            if (strcmp(crt_topic->name, message->topic) == 0) {
                send_data(iter->status, message, sizeof(struct message_udp) + 1);
            }
            crt_topic = crt_topic->next;
        }
        iter = iter->next;
    }
}

// adds a topic to a client's list of topics
void add_topic_for_client(struct client *head, struct client *crt_client, char *topic) {
    struct client *iter = head;
    while (iter != NULL) {
        if (iter == crt_client) {
            insert_topic_for_client(crt_client, topic);
            iter->no_of_topics++;
            break;
        }
        iter = iter->next;
    }
}

// removes a topic from a client's list of topics
void remove_topic_for_client(struct client *head, struct client *crt_client, char *topic) {
    struct client *iter = head;
    while (iter != NULL && iter != crt_client) {
        iter = iter->next;
    }
    if (iter == NULL) {
        return;
    }
    struct topic *crt_topic = iter->topics;
    while (crt_topic != NULL && strcmp(crt_topic->next->name, topic) != 0) {
        crt_topic = crt_topic->next;
    }
    if (crt_topic == NULL) {
        return;
    }
    struct topic *tmp = crt_topic->next;
    crt_topic->next = crt_topic->next->next;
    free(tmp);
}

// deallocates the memory
void free_list(struct client *head) {
    struct client *iter = head;

    while (iter != NULL) {
        struct client *tmp = iter;
        iter = iter->next;
        free(tmp->topics);
        free(tmp);
    }
}