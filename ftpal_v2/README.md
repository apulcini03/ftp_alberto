# Creazione di un programma `server.c` che rimane in ascolto di arrivo dati dalla rete inviati dal programma `client.c` che inizia la connessione

Questa versione aggiunge un client che sostituisce il comando "nc" per inviare dei dati e che stabilisce un protocollo di
comunicazione basato su dei messaggi per scambiare le informazioni con il server prima di inviare i dati (in forma di stringa)

Testata solo l'operazione di WRITE (da client a server)

SERVER:
WAITING_OPERATION
WAITING_SERVER_FILE_NAME
WAITING_DATA

CLIENT
Risponde ai messaggi precedenti inviando una string di esempio quando richiesto


NEL CLIENT Un'altra funzionalità aggiunta é la verifica dei parametri in INPUT con la funzione getopt