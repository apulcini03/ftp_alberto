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
    char buffer[BUF_SIZE];
    char buffer_out[BUF_SIZE];
    int n;

    while ((n = recv(new_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("Received: %s", buffer);
    }

    // Send welcome message
    strcpy(buffer_out, "Benvenuto ");
    strcat(buffer_out, buffer);
    send(new_socket, buffer_out, strlen(buffer_out), 0);

    //SIMULAZIONE ATTIVITA SERVER CON DATI RICEVUTI
    printf("Processing message %s Will end in 30 seconds\n\n\n", buffer);
    sleep(30);
    printf("End processing for %s\n", buffer);

    if (n < 0) {
        perror("Receive failed");
    }
}



// void handle_client(int client_socket) {
//     char buffer[BUF_SIZE];
//     int n;

//     // Send welcome message
//     strcpy(buffer, "220 Welcome to FTP server\r\n");
//     send(client_socket, buffer, strlen(buffer), 0);

//     while ((n = recv(client_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
//         buffer[n] = '\0';
//         printf("Received: %s", buffer);

//         if (strncmp(buffer, "USER", 4) == 0) {
//             strcpy(buffer, "331 Please specify the password.\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);
//         } else if (strncmp(buffer, "PASS", 4) == 0) {
//             strcpy(buffer, "230 Login successful.\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);
//         } else if (strncmp(buffer, "LIST", 4) == 0) {
//             strcpy(buffer, "150 Here comes the directory listing.\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);

//             // Simulate sending a directory listing
//             strcpy(buffer, "drwxr-xr-x 2 user group 4096 Jun  8 12:34 directory\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);

//             strcpy(buffer, "226 Directory send OK.\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);
//         } else if (strncmp(buffer, "QUIT", 4) == 0) {
//             strcpy(buffer, "221 Goodbye.\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);
//             break;
//         } else {
//             strcpy(buffer, "502 Command not implemented.\r\n");
//             send(client_socket, buffer, strlen(buffer), 0);
//         }
//     }

//     if (n < 0) {
//         perror("Receive failed");
//     }
// }