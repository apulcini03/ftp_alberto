# Creazione di un programma `server.c` che rimane in ascolto di arrivo dati dalla rete

Questa prima versione del programma crea solo un server che rimane in ascolto di messaggi di arrivo da un client su un socket la cui porta é 8080.
Il server stampa sul log del terminale il messaggio ricevuto ed invia una risposta al client.
Il server simula il lavoro per 30 secondi prima di chiudere la sessione

Per compilare ed avviare il programma

```bash
% gcc -o ftp_server server.c
% ./ftp_server              
Server is listening on port 8080
```

Il client é simulato con il comando unix: echo "paperino" | nc localhost 8080
per simulare più client:
- Aprire il primo terminale e digitare: echo "paperino" | nc localhost 8080
- Aprire il secondo terminale e digitare: echo "pluto" | nc localhost 8080

```bash
Client connected
Received: pluto
Processing message pluto
 Will end in 30 seconds


Client connected
Received: paperino
Processing message paperino
 Will end in 30 seconds


End processing for pluto

End processing for paperino
```

Un socket IP è un endpoint per l'invio o la ricezione di dati attraverso una rete di computer utilizzando il protocollo Internet (IP). È una tecnologia fondamentale per la comunicazione di rete nelle reti basate su IP, incluso Internet. Ecco una spiegazione più dettagliata:

Un socket IP è definito dai seguenti componenti:
- Indirizzo IP: Identifica un dispositivo specifico sulla rete (nel nostro caso un IP del server dove gira il software).
- Numero di Porta: é un numero che identifica una porta (logica) dell'IP prevedente al quale il nostro processo sarà legato (BINDING).

## La struttura `struct sockaddr_in` in C

La struttura `struct sockaddr_in` è utilizzata nella programmazione di socket per specificare gli indirizzi per la comunicazione IPv4 (Internet Protocol versione 4). È definita nell'header file `<netinet/in.h>`.

Un processo che gira su un server può essere messo in ascolto di dati in arrivo su un socket attraverso la struttura `sockaddr_in`

## Come funzionano i socket IP
- Creazione di un Socket: Un'applicazione crea un socket utilizzando chiamate di sistema come socket() in C, specificando la famiglia di indirizzi (es. AF_INET per IPv4), il tipo di socket (es. SOCK_STREAM per TCP, SOCK_DGRAM per UDP), e il protocollo.
- Binding: Il socket viene associato a un indirizzo IP locale e a un numero di porta utilizzando la chiamata di sistema bind().
- Ascolto e Accettazione delle Connessioni (solo TCP): Un socket server ascolta le connessioni in arrivo con listen() e le accetta con accept().
- Invio e Ricezione dei Dati: I dati vengono inviati e ricevuti utilizzando chiamate di sistema come send(), recv(), sendto(), e recvfrom().
- Chiusura del Socket: Una volta completata la comunicazione, il socket viene chiuso utilizzando la chiamata di sistema close().


### VEDERE ULTERIORI COMMENTI NEL CODICE
