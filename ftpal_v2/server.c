#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define BUF_SIZE 1024

void handle_client(int new_socket);


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

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
            handle_client(new_socket);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            close(new_socket);  // Close client socket in parent process
        }


    }

    close(server_fd);
    return 0;
}

void handle_client(int new_socket) {
    
    char *operation = NULL;
    char *server_file_path = NULL;
    char *state = "WAITING_OPERATION";

    char buffer[BUF_SIZE];
    char buffer_out[BUF_SIZE];
    int n;

    while ((n = recv(new_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("Data received: %s current state: %s\n", buffer, state); 

        //GESTISCO LO SCAMBIO DI MESSAGGI CON IL CLIENT
        if (strncmp(state, "WAITING_OPERATION", 18) == 0)  {
            if (strncmp(buffer, "WRITE", 6) == 0)  {
                operation = "WRITE";
                state = "WAITING_SERVER_FILE_NAME";
            } else if (strncmp(buffer, "READ", 5) == 0) {
                operation = "READ";
                state = "WAITING_SERVER_FILE_NAME";
            }
        } else if (strncmp(state, "WAITING_SERVER_FILE_NAME", 25) == 0)  {
            //prima di copiare una stringa (buffer[BUF_SIZE]) in un array di carattei dinamico, bisogna allocare la memoria per quest'ultimo
            server_file_path = malloc(strlen(buffer) + 1);
            strcpy(server_file_path, buffer);
            if (strncmp(operation, "WRITE", 6) == 0)  {
                state = "WAITING_DATA";
            } else if (strncmp(operation, "READ", 5) == 0)  {
                state = "SENDING_DATA";
            }
        } else if (strncmp(state, "WAITING_DATA", 13) == 0)  {
            printf("Server writing %s data to file: %s\n", buffer, server_file_path);
            //SIMULO LAVORO
            sleep(10);
            state = "COMPLETED";
        }

        printf("SERVER NEW state: %s operation %s server_file_path: %s\n", state, operation, server_file_path);   

        //INVIA MESSAGGIO AL CLIENT SE MESSAGGIO DIVERSO DA SENDING DATA
        if (strncmp(state, "SENDING_DATA", 13) != 0)  {
            strcpy(buffer_out, state);
            send(new_socket, buffer_out, strlen(buffer_out), 0);
            printf("Server sent STATE: %s\n", buffer_out);
            printf("\n");
        }
        //ALTRIMENT INVIA FILE
        else {
            printf("Server sending data from file: %s\n", server_file_path);
            printf("\n");
        }

    }

    if (n < 0) {
        perror("Receive failed");
    }
}



// void handle_client(int new_socket) {
//     char buffer[BUFFER_SIZE];
//     int bytes_read;
//     FILE *file;

//     // Apri un file per scrivere i dati ricevuti
//     file = fopen("received_file", "wb");
//     if (file == NULL) {
//         perror("fopen");
//         close(new_socket);
//         return;
//     }

//     // Ricevi i dati dal client e scrivili nel file
//     while ((bytes_read = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0) {
//         fwrite(buffer, 1, bytes_read, file);
//     }

//     if (bytes_read < 0) {
//         perror("recv");
//     }

//     printf("File received and saved as 'received_file'\n");

//     fclose(file);
//     close(new_socket);
//     printf("Client disconnected\n");
// }