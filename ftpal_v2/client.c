#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -w -a server_address -p port -f local_path/filename_local -o remote_path/filename_remote\n", prog_name);
    fprintf(stderr, "Usage: %s -r -a server_address -p port -f remote_path/filename_remote -o local_path/filename_local\n", prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

    int opt;
    char *operation = NULL;
    char *server_address = NULL;
    char *port = NULL;
    char *source_file_path = NULL;
    char *target_file_path = NULL;

    const char *optstring = "wr::a:p:f:o:";

    // Process options using getopt
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'w':
                operation = "WRITE";
                break;
            case 'r':
                operation = "READ";
                break;
            case 'a':
                server_address = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'f':
                source_file_path = optarg;
                break;
            case 'o':
                target_file_path = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    //IMPOSTA FILE_PATH MANCANTI
    if (source_file_path == NULL) {
        source_file_path = target_file_path;
    } else if (target_file_path == NULL) {
        target_file_path = source_file_path;
    }

    //VERIFICA CAMPI OBBLIGATORI
    if (operation == NULL) {
        printf("-w or -r option is missing \n");
        print_usage(argv[0]);
    }

    if (source_file_path == NULL) {
        printf("-f option is missing \n");
        print_usage(argv[0]);
    }

    if (server_address == NULL || port == NULL) {
        printf("-a or -p option is missing \n");
        print_usage(argv[0]);
    }

    //DEBUG
    printf("operation: %s\n", operation);
    printf("server_address: %s\n", server_address);
    printf("port: %s\n", port);
    printf("source_file_path: %s\n", source_file_path);
    printf("target_file_path: %s\n", target_file_path);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Crea il file descriptor del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converti l'indirizzo IP da testo a binario
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connettiti al server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // INVIA MESSAGGIO INIZIALE
    send(sock, operation, strlen(operation), 0);
    printf("Client sent: %s", operation);
    printf("\n");

    // Mettiti in ascolto infinito delle risposte del server
    char buffer_out[BUFFER_SIZE];
    int n;
    while ((n = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("Client received: %s", buffer);
        printf("\n");

        if (strncmp(buffer, "WAITING_SERVER_FILE_NAME", 25) == 0) {
            if (strncmp(operation, "WRITE", 5) == 0) {
                strcpy(buffer_out, target_file_path);
            } else if (strncmp(operation, "READ", 4) == 0) {
                strcpy(buffer_out, source_file_path);
            } else {
                printf("Error\n");
                break;
            }
            send(sock, buffer_out, strlen(buffer_out), 0);
            printf("Sent: %s", buffer_out);
            printf("\n");
        } else if (strncmp(buffer, "WAITING_DATA", 13) == 0) {
            printf("Client: Sending file data\n");
            //SIMULO CONTENUTO FILE
            char *file_content = "UNA SERIE DI INFORMAZIONI LETTE DAL CLIENT";
            strcpy(buffer_out, file_content);
            send(sock, buffer_out, strlen(buffer_out), 0);
            printf("Sent: %s", buffer_out);
            printf("\n");
        } else if (strncmp(buffer, "SENDING_DATA", 13) == 0) {
            printf("Client: Receiving Data\n");
        } else if (strncmp(buffer, "COMPLETED", 13) == 0) {
            printf("Done.\n");
            break;;
        }
    }


    //Chiudi il socket
    close(sock);

    return 0;
}
