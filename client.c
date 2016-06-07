#include "misc.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fatal_err("usage: %s hostname", argv[0]);
    }

    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(argv[1]);

    sockfd_t client_socket;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        fatal_err("socket failed");
    }

    if (connect(client_socket, (struct sockaddr *) &server, sizeof(server)) == -1) {
        close(client_socket);
        fatal_err("connect failed");
    }

    log_info("connecting to %s:%d", argv[1], PORT);

    while (1) {
        char message[MAXDATASIZE];

        log_info("enter message:"); printf("> "); fflush(stdout);
        if (scanf ("%[^\n]%*c", message) == -1) {
            break;
        }

        if (strstr(message, API_EXIT)) break;

        if(send_msg(client_socket, message, strlen(message)) == -1) {
            break;
        }

        char server_reply[MAXDATASIZE];
        long read_size;
        if((read_size = get_msg(client_socket , server_reply , MAXDATASIZE)) < 0)  {
            break;
        }

        if (read_size != 0) {
            log_info("server reply: \n%s", server_reply);
        }
    }

    close(client_socket);
    exit(EXIT_SUCCESS);
}
