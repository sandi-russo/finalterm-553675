# C-REST-Server: Un Server API REST Concorrente in C

Un server API RESTful leggero e performante, scritto in C puro, progettato per gestire richieste concorrenti utilizzando un'architettura I/O non bloccante con `epoll` e un backend **SQLite**.

---

## Introduzione

Questo progetto è un'implementazione *from-scratch* di un server HTTP che espone un'API REST per semplici operazioni CRUD (Create, Read, Delete) su una risorsa "utenti".

Il cuore dell'architettura si basa su un design **event-driven** ad alte prestazioni che unisce:

* **I/O Multiplexing** con `epoll` (sul thread principale) per gestire migliaia di connessioni simultanee in modo efficiente e non bloccante.
* Un **Thread Pool** (basato su Pthreads) per l'elaborazione delle richieste.

Questo modello (simile al design Reactor/Proactor) assicura che il thread I/O principale non venga mai bloccato, delegando il lavoro pesante (parsing, query al database) ai thread worker, massimizzando così la scalabilità e la reattività del server.

---

## Caratteristiche

* **Server HTTP Concorrente:** Gestisce più client contemporaneamente.
* **Architettura Scalabile:** Utilizza `epoll` (Linux) per I/O non bloccante e un pool di thread Pthreads.
* **API RESTful:** Espone endpoint CRUD per una risorsa "utenti".
* **Persistenza Dati:** Utilizza **SQLite 3** come backend di database.
* **Leggero:** Scritto in C puro, con dipendenze minime.
* **Gestione Pulita:** Compilazione gestita tramite `Makefile`.

---

## Prerequisiti

Per compilare ed eseguire il server, sono necessari:

* `gcc` (o `clang`)
* `make`
* La libreria di sviluppo di **SQLite3** (es. `libsqlite3-dev` su Debian/Ubuntu o `sqlite-devel` su Fedora)
* La libreria di sviluppo di **Pthreads** (solitamente inclusa in `build-essential` o `libc-dev`)

---

## Compilazione

Il progetto include un `Makefile` per una compilazione semplice.

1.  Clona il repository:
```
git clone https://github.com/sandi-russo/finalterm-553675.git
cd finalterm-553675
```

2.  Compila il progetto:
```
make
```

3.  Questo creerà un eseguibile chiamato `rest_server`.

Sono disponibili anche altri comandi `make`:

* `make clean`: Rimuove i file oggetto e l'eseguibile.
* `make re`: Esegue `clean` e poi `all` per una ricompilazione completa.

---

## Esecuzione

Una volta compilato, avvia il server:

```
./rest_server
```

Il server si avvierà e si metterà in ascolto sulla porta `8080` (come definito in `common.h`). Verrà creato automaticamente un file di database `rest_api.db` se non esiste.

```
$ ./rest_server
Database inizializzato e tabella 'users' pronta.
Thread pool creato con 4 workers.
Server REST in ascolto sulla porta 8080...
```

---

## Utilizzo (API Endpoints)

È possibile testare l'API utilizzando `curl` da un altro terminale.

### 1. Creare un utente (POST /users)

Il body deve essere in formato `name=...` (come gestito da `db_handler.c`).

```
curl -X POST -d "name=Sandi" http://localhost:8080/users
# Risposta: {"status":"created"}

curl -X POST -d "name=Mario" http://localhost:8080/users
# Risposta: {"status":"created"}
```

### 2. Ottenere tutti gli utenti (GET /users)

```
curl http://localhost:8080/users
# Risposta: [{"id":1, "name":"Sandi"},{"id":2, "name":"Mario"}]
```

### 3. Ottenere un utente singolo (GET /users/:id)

```
curl http://localhost:8080/users/1
# Risposta: {"id":1, "name":"Sandi"}
```

### 4. Eliminare un utente (DELETE /users/:id)

```
curl -X DELETE http://localhost:8080/users/1
# Risposta: {"status":"deleted"}
```

### 5. Controllare gli utenti rimanenti

```
curl http://localhost:8080/users
# Risposta: [{"id":2, "name":"Mario"}]
```

### 6. Endpoint non trovato (404)

```
curl http://localhost:8080/non-esiste
# Risposta: Endpoint non trovato
```

---

## Struttura del Progetto

```
.
├── server.c         # Loop principale, I/O con epoll (Reactor)
├── http_handler.c   # Logica di parsing HTTP, routing e risposta
├── http_handler.h   # Header per il gestore HTTP
├── thread_pool.c    # Implementazione del Thread Pool
├── thread_pool.h    # Header del Thread Pool
├── db_handler.c     # Gestore del database (SQLite3)
├── db_handler.h     # Header del gestore DB
├── common.h         # Definizioni e costanti comuni
├── Makefile         # Script di compilazione
└── rest_api.db      # (File di database generato all'avvio)
```

---

## Licenza

Questo progetto è distribuito sotto la licenza MIT. Vedi il file `LICENSE` per maggiori dettagli.
