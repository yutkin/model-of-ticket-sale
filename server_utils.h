#ifndef OSLAB5_SERVER_UTILS_H
#define OSLAB5_SERVER_UTILS_H

#define QUEUE_SIZE 10	// the number of pending connections that queue will hold
#define BOUGHT 1
#define AVAILABLE 0
#define SHARED_MEMORY_NAME "SHARED_MEM"
#define MUTEX "/mutex"

static char PLANE_SCHEMA[] = \
"                      +--+\n"
        "                      | * \\\n"
        "                      | *  \\\n"
        "                  +---+ *   \\\n"
        "                  +---+ *    \\\n"
        "+---+                 | *     \\\n"
        "| *  \\  +---------------------------------------+\n"
        "+ *   \\ | %d |    %d |    %d |    %d |    %d | | \\\n"
        "\\  *   \\| %d |    %d |    %d |    %d |    %d | |  \\\n"
        " \\  *   | %d |    %d |    %d |    %d |    %d | |   \\\n"
        "  +-----+                                      |  --\\\n"
        "  +-----+                                      |  --/\n"
        " /  *   | %d |    %d |    %d |    %d |    %d | |   /\n"
        "/  *   /| %d |    %d |    %d |    %d |    %d | |  /\n"
        "+ *   / | %d |    %d |    %d |    %d |    %d | | /\n"
        "| *  /  +---------------------------------------+\n"
        "+---+                 | *     /\n"
        "                  +---+ *    /\n"
        "                  +---+ *   /\n"
        "                      | *  /\n"
        "                      | * /\n"
        "                      +--+";


struct order {
    int ticket_id;
    char *customer;
};

struct ticket {
    int smoking;
    int bought;
    char customer[40];
};

struct database {
    struct ticket tickets[30];
};

static sem_t *mutex;


struct database *db_cursor() {
    if (shm_unlink(SHARED_MEMORY_NAME) != 0) {
        log_debug("shm_unlink");
    }

    int fd = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd == -1) {
        fatal_err("shm_open failed");
    }

    if (ftruncate(fd, sizeof(struct database)) != 0) {
        fatal_err("ftruncate failed");
    }

    struct database *db = (struct database *) mmap(0, sizeof(struct database),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (db == MAP_FAILED) {
        fatal_err("mmap failed");
    }
    close(fd);

    for (int i = 0; i < TOTAL_TICKETS; ++i) {
        db->tickets[i].smoking = (i % 5 == 0) ? 1 : 0;
        db->tickets[i].bought = AVAILABLE;
    }
    return db;
}

struct order *parse_buying_request(char *request) {
    struct order *order = (struct order *) malloc(sizeof(struct order));
    char *i = strchr(request, ' '); // jump to space
    if (!i) return NULL;
    while (*i == ' ') ++i; // skip next spaces

    // parse order id
    char *ptr;
    long id = strtol(i, &ptr, 10);
    if (id > 0 && id <= TOTAL_TICKETS)
        order->ticket_id = id-1;
    else
        return NULL;
    i = ptr;

    while (*i == ' ') ++i; // skip next spaces
    size_t rest_sz = strlen(i);
    if (rest_sz == 0)
        return NULL;
    order->customer = (char *) malloc(rest_sz+1);
    if (order->customer) {
        strcpy(order->customer, i);
        return order;
    }
    return NULL;
}

sockfd_t init_server() {
    int master_socket;
    if ((master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        fatal_err("socket failed");
    }

    int yes = 1;
    if (setsockopt(master_socket,
                   SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        fatal_err("setsockopt SO_REUSEADDR failed");
    }

    struct sockaddr_in socket_address;
    bzero(&socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(PORT);
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(master_socket,
             (struct sockaddr *) &socket_address, sizeof(socket_address)) < 0)
    {
        fatal_err("bind failed");
    }


    if (listen(master_socket, QUEUE_SIZE) == -1) {
        fatal_err("listen failed");
    }
    return master_socket;
}

sem_t *init_semaphore(int val) {
    sem_t *sem;
    if (sem_unlink(MUTEX) != 0) {
        log_debug("shm_unlink");
    }

    if ((sem = sem_open(MUTEX, O_CREAT, 0644, 1)) == SEM_FAILED) {
        fatal_err("sem_open failed");
    }
    return sem;
}

size_t avail_tick(const struct database *db) {
    size_t n = 0;
    for (int i = 0; i < TOTAL_TICKETS; ++i) {
        if (db->tickets[i].bought == AVAILABLE) {
            ++n;
        }
    }
    return n;
}

size_t get_list_of_passengers(const struct database *db, char *buf) {
    for (int i = 0; i < TOTAL_TICKETS; ++i) {
        if (db->tickets[i].bought == BOUGHT) {
            strcat(buf, db->tickets[i].customer);
            strcat(buf, "\n");
        }
    }
    size_t bufl = strlen(buf);
    if (bufl > 0) {
        buf[bufl-1] = '\0';
    }
    return bufl;
}

void get_list_of_smoking(const struct database *db, char *buf) {
    for (int i = 0; i < TOTAL_TICKETS; ++i) {
        if (db->tickets[i].smoking) {
            char tmp[4];
            sprintf(tmp, "%d ", i+1);
            strcat(buf, tmp);
        }
    }
}

void get_schema(const struct database *db, char *buf) {
    char tmp[255]= { 0 };
    char smoking[30] = { 0 };
    get_list_of_smoking(db, smoking);

    sprintf(tmp, "\n\tTotal seats: %d\n"
            "\tSmoking seats: %s\n"
            "\tAvailable tickets: %lu", TOTAL_TICKETS, smoking, avail_tick(db));
    int ith_ticket = 0;
    int i = 0;
    while (i < sizeof PLANE_SCHEMA) {
        if (PLANE_SCHEMA[i] != '%') {
            buf[i] = PLANE_SCHEMA[i];
            ++i;
        } else {
            if (db->tickets[ith_ticket].bought == AVAILABLE)
                sprintf(buf + i, "%02d", ith_ticket + 1);
            else
                sprintf(buf + i, "XX");
            ++ith_ticket;
            i += 2;
        }
    }
    strcat(buf, tmp);
}

void handle_client(sockfd_t client_fd, struct database *db, sem_t *mutex, char *client_ip) {
    while (1) {
        char message[MAXDATASIZE];
        long read_size;
        if ((read_size = get_msg(client_fd, message, MAXDATASIZE)) <= 0) {
            break;
        }
        log_debug("got msg from %s: \"%s\"", client_ip, message);

        if (strstr(message, API_GET_SCHEMA)) {
            char buf[sizeof PLANE_SCHEMA];
            get_schema(db, buf);
            send_msg(client_fd, buf, strlen(buf));

        } else if (strstr(message, API_GET_SMOKING)) {
            char buf[30] = { 0 };
            get_list_of_smoking(db, buf);
            send_msg(client_fd, buf, strlen(buf));

        } else if (strstr(message, API_GET_PASSENGERS)) {
            char buf[MAXDATASIZE] = { 0 };
            size_t bufsz;
            if ((bufsz = get_list_of_passengers(db, buf)))
                send_msg(client_fd, buf, bufsz);
            else
                send_msg(client_fd, "There is no passengers yet.", 27);

        } else if (strstr(message, API_BUY_TICKET)) {
            struct order *order = parse_buying_request(message);
            if (order) {
                int id = order->ticket_id;
                int val = 0;

                sem_wait(mutex);
                log_debug("%d captured mutex", getpid());

                if (db->tickets[id].bought != BOUGHT) {
                    db->tickets[id].bought = BOUGHT;
                    strcpy(db->tickets[id].customer, order->customer);
                    send_msg(client_fd, "OK", 2);
                } else {
                    char msg[] = "This seat has been bought.";
                    send_msg(client_fd, msg, strlen(msg));
                }

                log_debug("%d released mutex", getpid());
                free(order);

                sem_post(mutex);

            } else {
                char msg[] = "wrong arguments.";
                char buf[sizeof(API_BUY_TICKET)+ sizeof(msg)+1];
                sprintf(buf, "%s: %s", API_BUY_TICKET, msg);
                send_msg(client_fd, buf, strlen(buf));
            }

        } else if (strstr(message, API_EXIT)) {
            break;

        } else {
            char buf[strlen(message) + 34];
            sprintf(buf, "\"%s\" - unknown command, check help.", message);
            send_msg(client_fd, buf, strlen(message) + 34);
        }
    }
}

void sigchld_handler(int s) {
    pid_t child_pid;
    while((child_pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        log_debug("child %d terminated", child_pid);
    }
}

void sigint_handler(int s) {
    log_info("Keyboard interrupt received, exiting...");
    sem_close(mutex);
    sem_unlink(MUTEX);
    exit(EXIT_SUCCESS);
}

void get_client_address(struct sockaddr_storage *client_addr, char *buf) {
    inet_ntop(client_addr->ss_family,
              get_in_addr((struct sockaddr *) client_addr), buf, INET6_ADDRSTRLEN);
}

sockfd_t accept_new_connection(sockfd_t master_socket, struct sockaddr_storage *client_addr) {
    sockfd_t client_fd;  // listen on sock_fd, new connection on client_fd

    socklen_t sin_size = sizeof(*client_addr);
    client_fd = accept(master_socket, (struct sockaddr *) client_addr, &sin_size);
    if (client_fd == -1) {
        log_err("master socket accept failed");
        return -1;
    }
    return client_fd;
}

#endif //OSLAB5_SERVER_UTILS_H
