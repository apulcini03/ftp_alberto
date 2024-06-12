#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 8080
#define BUF_SIZE 1024

void handle_client(int new_socket, char *ft_root_directory);

void send_status(int client_socket, char *status);

void create_directories(const char *path);

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -a server_address -p port -d ft_root_directory\n", prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    char *server_address = NULL;
    char *port = NULL;
    char *ft_root_directory = NULL;


    const char *optstring = "a:p:d:";
    // Process options using getopt
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'a':
                server_address = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'd':
                ft_root_directory = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    //CREAZIONE ROOT DIRECTORY SE NON ESISTE
    struct stat st = {0};

    if (stat(ft_root_directory, &st) == -1) {
        if (mkdir(ft_root_directory, 0700) != 0) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        } else {
            printf("Server working directory %s created\n", ft_root_directory);
        }
    } else {
        printf("Server working directory %s already exists\n", ft_root_directory);
    }


    // Creazione del socket: La funzione socket() crea un socket e restituisce un file descriptor. I parametri:
    // AF_INET: Specifica che il socket utilizza l'indirizzo IPv4.
    // SOCK_STREAM: Specifica che il socket utilizza TCP (flusso di byte affidabile).
    // 0: Protocollo predefinito.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("errore socket()");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    // Impostazione delle opzioni del socket: La funzione setsockopt() imposta le opzioni del socket:
    // SOL_SOCKET: Livello delle opzioni del socket.
    // SO_REUSEADDR | SO_REUSEPORT: Permette di riutilizzare l'indirizzo e la porta.
    // &opt: Puntatore alla variabile che contiene l'opzione.
    // sizeof(opt): Dimensione della variabile.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == 0) {
        perror("errore setsockopt()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurazione della struttura sockaddr_in:
    // sin_family: Imposta la famiglia di indirizzi a AF_INET (IPv4).
     // sin_port: Imposta il numero di porta convertito in ordine di byte di rete utilizzando htons().
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    // sin_addr.s_addr: Imposta l'indirizzo IP del server a INADDR_ANY, che significa che il server ascolterà su tutte le interfacce disponibili.
    address.sin_addr.s_addr = INADDR_ANY;
    // In alternativa al precedente si può convertire un IP address da testo
    // NON FUNZIONA: DA DEBUGGARE 
    // verificare l'IP del server su cui gira il programma. 
    // es:
    // % ifconfig | grep 192.168
    //         inet 192.168.1.56 netmask 0xffffff00 broadcast 192.168.1.255
    // % 
    // if (inet_pton(AF_INET, "192.168.1.56", &address.sin_addr) <= 0) {
    //     perror("errore impostazione IP inet_pton()");
    //     close(server_fd);
    //     exit(EXIT_FAILURE);
    // }

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("Client connected\n");

        //PER GESTIRE I CLIENT IN SEQUENZA CHIAMARE LA FUNZIONE handle_client
        // Handle the client (for example, you could use handle_client(new_socket))
        // handle_client(new_socket);
        // close(new_socket);
                
        //PER GESTIRE PIU' CLIENT IN PARALELLO bisogna utilizzare una fork() che crea un nuovo processo per pgni chiamata all funzione handle_client()
        //il risultato sarà che dal prompt unix vedremo tanti processi quanti sono i client attivi
        // % ps -ef | grep ftp_server
        //   502 55409 46634   0  7:17pm ttys003    0:00.00 grep ftp_server    << COMANDO GREP
        //   502 55291 22723   0  7:16pm ttys005    0:00.00 ./ftp_server        << COMANDO PRINCIPALE ID 55291
        //   502 55370 55291   0  7:16pm ttys005    0:00.00 ./ftp_server        << CHIAMAT FORK ID PARENT 55291
        //   502 55396 55291   0  7:16pm ttys005    0:00.00 ./ftp_server        << CHIAMAT FORK ID PARENT 55291
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(new_socket);
            continue;
        } else if (pid == 0) {
            // Child process
            close(server_fd);  // Close server socket in child process
            handle_client(new_socket, ft_root_directory);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            close(new_socket);  // Close client socket in parent process
        }


    }

    close(server_fd);
    return 0;
}

void handle_client(int new_socket, char *ft_root_directory) {
    
    char *operation = NULL;
    char *server_file_path = NULL;
    char *status = "WAITING_OPERATION";

    char buffer[BUF_SIZE];
    int n;

    FILE *file;

    //DEBUG
    printf("Server STATUS (initial): %s %d\n", status, new_socket);


    while ((n = recv(new_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';

        //DEBUG STATO
        if (strncmp(status, "SERVER_RECEIVING_DATA", 22) != 0 || strncmp(buffer, "WRITE_COMPLETED", 16) == 0)   {
            printf(" Data received from clien: %s\n", buffer);  
        }

        //GESTISCO LO SCAMBIO DI MESSAGGI CON IL CLIENT
        if (strncmp(status, "WAITING_OPERATION", 18) == 0)  {
            if (strncmp(buffer, "WRITE", 6) == 0)  {
                operation = "WRITE";
                status = "WAITING_SERVER_FILE_NAME";
            } else if (strncmp(buffer, "READ", 5) == 0) {
                operation = "READ";
                status = "WAITING_SERVER_FILE_NAME";
            }
            send_status(new_socket, status);

        } else if (strncmp(status, "WAITING_SERVER_FILE_NAME", 25) == 0)  {
            //prima di copiare una stringa (buffer[BUF_SIZE]) in un array di carattei dinamico, bisogna allocare la memoria per quest'ultimo
            // Calculate the total length required for the concatenated string
            size_t total_length = strlen(ft_root_directory) + 1 + strlen(buffer) + 1; // +1 for the null terminator
            // Allocate memory for the concatenated string
            //char *server_file_path = (char *)malloc(total_length * sizeof(char));
            server_file_path = malloc(strlen(ft_root_directory) + 1 + strlen(buffer) + 1);
            if (server_file_path == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                break;
            }
            strcpy(server_file_path, ft_root_directory);
            strcat(server_file_path, "/");
            strcat(server_file_path, buffer);
            printf("Server side file full path: %s\n", server_file_path);

            create_directories(server_file_path);

            // Apri un file per SCRIVERE O LEGGERE i dati
            file = fopen(server_file_path, "wb");
            if (file == NULL) {
                perror("fopen");
                //GESTIRE EVENTUALE ERRORE
            }

            if (strncmp(operation, "WRITE", 6) == 0)  {
                status = "SERVER_RECEIVING_DATA";
            } else if (strncmp(operation, "READ", 5) == 0)  {
                status = "SERVER_SENDING_DATA";
            }

            send_status(new_socket, status);

        } else if (strncmp(status, "SERVER_RECEIVING_DATA", 22) == 0)  {
            if (strncmp(buffer, "WRITE_COMPLETED", 16) == 0)  {
                sleep(10);
                fclose(file);
                printf("Server writing completed %s\n\n", server_file_path);
            } else {
                printf("Server writing in file %s\n", server_file_path);
                fwrite(buffer, 1, n, file);
            }

        }

    }

    if (n < 0) {
        perror("Receive failed");
    }


}

void send_status(int client_socket, char *status) {
    char buffer_out[BUF_SIZE];
    strcpy(buffer_out, status);
    send(client_socket, buffer_out, strlen(buffer_out), 0);
    printf("Server sent STATUS: %s\n", buffer_out);
}

void create_directories(const char *path) {
    char temp[1024];
    char *pos = temp;

    // Copy the path to a temporary buffer
    strncpy(temp, path, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';  // Ensure null-termination

    // Iterate through the path
    while (*pos) {
        if (*pos == '/' || *pos == '\\') {
            *pos = '\0';

            //printf("  create dir: %s\n", temp);
            // Skip the drive letter in Windows paths
            if (strlen(temp) > 0 && (strlen(temp) != 2 || temp[1] != ':')) {
                // Check if directory exists, if not create it
                if (mkdir(temp, 0755) != 0 && errno != EEXIST) {
                    fprintf(stderr, "Error creating directory %s: %s\n", temp, strerror(errno));
                    return;
                }
            }
            *pos = '/';  // Restore the separator
        }
        pos++;
    }

}

