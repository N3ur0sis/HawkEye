/*
 * orchestrateur.c
 * Prototype minimal : 
 *  - Se connecte à l'agent (server TCP).
 *  - Envoie une commande (par ex. "ls -l").
 *  - Reçoit la sortie d'exécution de l'agent et l'affiche.
 *
 * Compilation : gcc orchestrateur.c -o orchestrateur
 * Exécution   : ./orchestrateur <IP_AGENT> <PORT> "<COMMANDE>"
 *
 * Exemple :
 *   ./orchestrateur 127.0.0.1 9999 "ls -l"
 */

 #include <stdio.h>      // printf, perror
 #include <stdlib.h>     // exit
 #include <string.h>     // memset, strlen
 #include <unistd.h>     // close, read, write
 #include <arpa/inet.h>  // htons, inet_addr
 #include <sys/socket.h> // socket, connect
 #include <netinet/in.h> // sockaddr_in
 
 int main(int argc, char* argv[]) {
     if (argc < 4) {
         fprintf(stderr, "Usage: %s <IP_AGENT> <PORT> \"<COMMANDE>\"\n", argv[0]);
         return 1;
     }
 
     // Récupération des arguments
     //TODO: Passer les details des agents via un fichier de config
     const char* ip_agent   = argv[1];
     int         port_agent = atoi(argv[2]);
     const char* commande   = argv[3];
 
     // 1) Création du socket TCP (client)
     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
         perror("Erreur socket()");
         exit(EXIT_FAILURE);
     }
 
     // 2) Préparation de l'adresse de l'agent
     struct sockaddr_in agent_addr;
     memset(&agent_addr, 0, sizeof(agent_addr));
     agent_addr.sin_family      = AF_INET;
     agent_addr.sin_port        = htons(port_agent); //Conversion host order to network order
     agent_addr.sin_addr.s_addr = inet_addr(ip_agent);  // Conversion de l'IP en binaire
 
     // 3) Connexion à l'agent
     if (connect(sockfd, (struct sockaddr*)&agent_addr, sizeof(agent_addr)) < 0) {
         perror("Erreur connect()");
         close(sockfd);
         exit(EXIT_FAILURE);
     }
     printf("[Orchestrateur] Connecté à l'agent %s:%d\n", ip_agent, port_agent);
 
     // 4) Envoi de la commande à l'agent
     ssize_t sent = write(sockfd, commande, strlen(commande));
     if (sent < 0) {
         perror("Erreur write() commande");
         close(sockfd);
         exit(EXIT_FAILURE);
     }
     // On write  un '\n' pour signaler la fin de la commande
     write(sockfd, "\n", 1);
 
     printf("[Orchestrateur] Commande envoyée : %s\n", commande);
 
     // 5) Réception de la sortie d'exécution
     //    On va lire jusqu'à ce que l'agent ferme la connexion ou qu'on reçoive 0
     char buffer[1024];
     memset(buffer, 0, sizeof(buffer));
 
     printf("[Orchestrateur] Sortie de l'agent :\n");
 
     ssize_t received;
     while ((received = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
         buffer[received] = '\0';  // on check que c'est une chaîne duh 
         printf("%s", buffer);
         memset(buffer, 0, sizeof(buffer));
     }
     
     // 6) Exit
     printf("\n[Orchestrateur] Fin de la réception.\n");
     close(sockfd);
 
     return 0;
 }