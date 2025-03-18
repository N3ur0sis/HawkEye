/*
 * agent.c
 * Prototype minimal :
 *  - Écoute en TCP sur un port donné.
 *  - Accepte la connexion de l'orchestrateur.
 *  - Lit la commande envoyée.
 *  - L'exécute en local (via popen).
 *  - Récupère la sortie et la renvoie à l'orchestrateur.
 *
 * Compilation : gcc agent.c -o agent
 * Exécution   : ./agent <PORT>
 *
 * Exemple :
 *   ./agent 9999
 */

 #include <stdio.h>      // printf, perror, FILE, pclose
 #include <stdlib.h>     // exit
 #include <string.h>     // memset, strlen
 #include <unistd.h>     // close, read, write
 #include <arpa/inet.h>  // htons, htonl, etc.
 #include <sys/socket.h> // socket, bind, listen, accept
 #include <netinet/in.h> // sockaddr_in
 
 int main(int argc, char* argv[]) {
     if (argc < 2) {
         fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
         return 1;
     }
 
     int port = atoi(argv[1]);
 
     // 1) Création du socket (serveur TCP)
     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server_fd < 0) {
         perror("Erreur socket()");
         exit(EXIT_FAILURE);
     }
 
     // 2) Liaison (bind) sur l'adresse locale, port spécifié
     struct sockaddr_in serv_addr;
     memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family      = AF_INET;
     serv_addr.sin_port        = htons(port);
     serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 
     if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("Erreur bind()");
         close(server_fd);
         exit(EXIT_FAILURE);
     }
     printf("[Agent] Bind sur le port %d\n", port);
 
     // 3) Mise en écoute
     if (listen(server_fd, 5) < 0) {
         perror("Erreur listen()");
         close(server_fd);
         exit(EXIT_FAILURE);
     }
     printf("[Agent] En écoute...\n");
 
     // 4) Acceptation de la connexion de l'orchestrateur
     struct sockaddr_in client_addr;
     socklen_t client_len = sizeof(client_addr);
     int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
     if (client_fd < 0) {
         perror("Erreur accept()");
         close(server_fd);
         exit(EXIT_FAILURE);
     }
     printf("[Agent] Orchestrateur connecté.\n");
 
     // 5) Lecture de la commande envoyée
     char command[1024];
     memset(command, 0, sizeof(command));
 
     ssize_t n = read(client_fd, command, sizeof(command) - 1);
     if (n <= 0) {
         perror("Erreur read() commande");
         close(client_fd);
         close(server_fd);
         exit(EXIT_FAILURE);
     }
     command[n] = '\0';
     printf("[Agent] Commande reçue : %s\n", command);
 
     // 6) Exécution de la commande via popen() pour récupérer la sortie
     FILE* fp = popen(command, "r"); 
     if (!fp) {
         perror("Erreur popen()");
         // on peut envoyer un message d'erreur à l'orch si on veut
         close(client_fd);
         close(server_fd);
         exit(EXIT_FAILURE);
     }
 
     // 7) Lecture de la sortie de la commande et envoi à l'orchestrateur
     char output[1024];
     memset(output, 0, sizeof(output));
 
     while (fgets(output, sizeof(output), fp) != NULL) {
         // envoie ligne par ligne vers le client
         write(client_fd, output, strlen(output));
         memset(output, 0, sizeof(output));
     }
 
     pclose(fp);
     printf("[Agent] Fin de l'exécution de la commande.\n");
 
     // 8) Exit
     close(client_fd);
     close(server_fd);
 
     return 0;
 }