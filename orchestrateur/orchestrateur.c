/*
 * orchestrateur_multi_agent.c
 * Prototype :
 *  - Lit un fichier "agents.conf" contenant la liste des agents.
 *  - Gère plusieurs agents en parallèle avec pthread.
 *  - Envoie une commande à chaque agent et affiche la réponse.
 *
 * Compilation : gcc orchestrateur_multi_agent.c -o orchestrateur -lpthread
 * Exécution   : ./orchestrateur agents.conf
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <pthread.h>
 
 #define BUFFER_SIZE 1024
 #define MAX_AGENTS 10  // Limite d'agents simultanés
 
 // Structure pour stocker les informations d'un agent
 typedef struct {
     char ip[16];    // Adresse IP
     int port;       // Port de connexion
     char commande[256]; // Commande à exécuter
 } AgentInfo;
 
 // Fonction exécutée par chaque thread pour communiquer avec un agent
 void* gerer_agent(void* arg) {
     AgentInfo* agent = (AgentInfo*) arg;
 
     // Création du socket TCP client
     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
         perror("[Erreur] socket()");
         pthread_exit(NULL);
     }
 
     // Configuration de l'adresse de l'agent
     struct sockaddr_in agent_addr;
     memset(&agent_addr, 0, sizeof(agent_addr));
     agent_addr.sin_family = AF_INET;
     agent_addr.sin_port = htons(agent->port);
     agent_addr.sin_addr.s_addr = inet_addr(agent->ip);
 
     // Connexion à l'agent
     if (connect(sockfd, (struct sockaddr*)&agent_addr, sizeof(agent_addr)) < 0) {
         perror("[Erreur] connect()");
         close(sockfd);
         pthread_exit(NULL);
     }
     printf("[Orchestrateur] Connecté à %s:%d\n", agent->ip, agent->port);
 
     // Envoi de la commande
     ssize_t sent = write(sockfd, agent->commande, strlen(agent->commande));
     if (sent < 0) {
         perror("[Erreur] write() commande");
         close(sockfd);
         pthread_exit(NULL);
     }
     write(sockfd, "\n", 1); // Signal de fin de commande
 
     printf("[Orchestrateur] Commande envoyée à %s : %s\n", agent->ip, agent->commande);
 
     // Réception de la sortie de l'agent
     char buffer[BUFFER_SIZE];
     memset(buffer, 0, sizeof(buffer));
 
     printf("[Orchestrateur] Réponse de %s :\n", agent->ip);
     ssize_t received;
     while ((received = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
         buffer[received] = '\0';
         printf("%s", buffer);
         memset(buffer, 0, sizeof(buffer));
     }
 
     printf("\n[Orchestrateur] Fin de réception de %s\n", agent->ip);
     close(sockfd);
     pthread_exit(NULL);
 }
 
 // Fonction pour lire le fichier de config et lancer les threads
 void charger_agents(const char* fichier_config) {
     FILE* file = fopen(fichier_config, "r");
     if (!file) {
         perror("[Erreur] Impossible d'ouvrir le fichier d'agents");
         exit(EXIT_FAILURE);
     }
 
     AgentInfo agents[MAX_AGENTS];
     int count = 0;
 
     // Lecture des agents depuis le fichier
     while (fscanf(file, "%15s %d \"%255[^\"]\"", agents[count].ip, &agents[count].port, agents[count].commande) == 3) {
         count++;
         if (count >= MAX_AGENTS) break; // Éviter de dépasser la limite
     }
     fclose(file);
 
     printf("[Orchestrateur] Chargé %d agents.\n", count);
 
     // Lancer un thread par agent
     pthread_t threads[MAX_AGENTS];
     for (int i = 0; i < count; i++) {
         if (pthread_create(&threads[i], NULL, gerer_agent, &agents[i]) != 0) {
             perror("[Erreur] pthread_create()");
         }
     }
 
     // Attendre la fin de tous les threads
     for (int i = 0; i < count; i++) {
         pthread_join(threads[i], NULL);
     }
 
     printf("[Orchestrateur] Tous les agents ont terminé.\n");
 }
 
 int main(int argc, char* argv[]) {
     if (argc < 2) {
         fprintf(stderr, "Usage: %s <fichier_config>\n", argv[0]);
         return 1;
     }
 
     charger_agents(argv[1]);
     return 0;
 }