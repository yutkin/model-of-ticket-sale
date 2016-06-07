#include "misc.h"
#include "server_utils.h"

int main(int argc, char *argv[]) {

    struct database *db = db_cursor();
    sockfd_t master_socket = init_server();
    mutex = init_semaphore(1);

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    log_info("waiting for connections...");
    while (1) {
        struct sockaddr_storage client_addr; // connector's address information
        sockfd_t client_fd;  // listen on sock_fd, new connection on client_fd
        client_fd = accept_new_connection(master_socket, &client_addr);

        char client_ip[INET6_ADDRSTRLEN];
        get_client_address(&client_addr, client_ip);
        log_info("got connection from %s", client_ip);

        if (!fork()) {
            log_debug("child %d created", getpid());
            signal(SIGINT, SIG_DFL);
            close(master_socket);
            handle_client(client_fd, db, mutex, client_ip);
            close(client_fd);
            log_info("%s disconnected", client_ip);
            exit(EXIT_SUCCESS);
        }
        close(client_fd);
    }
}