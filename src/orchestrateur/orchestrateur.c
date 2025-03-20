#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

/* Configuration constants */
#define MAX_AGENTS 10        // Maximum number of concurrent agents
#define BUFFER_SIZE 1024     // Network buffer size for socket operations
#define RESPONSE_SIZE 4096   // Maximum response size from agents

/* NOTE: Consider making these configurable via config file */

/**
 * Agent network endpoint representation
 * Keeps memory footprint minimal with fixed-size arrays
 */
typedef struct {
    char ip[16];    // IPv4 address in string format
    int port;
} AgentInfo;

/**
 * Task execution context for each agent
 * Command and response are kept in same structure for thread safety
 */
typedef struct {
    AgentInfo agent;
    char command[512];
    char response[RESPONSE_SIZE];
} AgentTask;

/* Global state */
AgentInfo agents[MAX_AGENTS];
int agent_count = 0;

/* TODO: Consider implementing mutex locks for thread-safe agent management */
/* TODO: Implement agent health checking mechanism */
/* TODO: Add support for agent authentication */

/**
 * Loads agent configuration from file
 * Format: <ip> <port> per line
 * Silently truncates if more agents than MAX_AGENTS are specified
 */
void load_agents(const char *filename) {
    FILE *file = fopen(filename, "r");
    if(!file) { 
        perror("Error opening agents file"); 
        exit(1); 
    }
    
    agent_count = 0;
    while(agent_count < MAX_AGENTS && 
          fscanf(file, "%15s %d", agents[agent_count].ip, &agents[agent_count].port) == 2) {
        agent_count++;
    }
    fclose(file);
}

/**
 * Thread worker function for command execution
 * Implements a simple protocol:
 * 1. Connect to agent
 * 2. Send command followed by newline
 * 3. Read response until connection closes
 * 
 * NOTE: Current implementation doesn't handle partial writes
 * TODO: Implement timeout mechanism
 * TODO: Add error reporting mechanism back to main thread
 */
void *send_command(void *arg) {
    AgentTask *task = (AgentTask*)arg;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) { 
        pthread_exit(NULL); 
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(task->agent.port);
    addr.sin_addr.s_addr = inet_addr(task->agent.ip);

    if(connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { 
        close(sockfd); 
        pthread_exit(NULL); 
    }

    /* Protocol implementation */
    write(sockfd, task->command, strlen(task->command));
    write(sockfd, "\n", 1);

    /* Accumulate response with overflow protection */
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int total = 0;
    while(1) {
        ssize_t n = read(sockfd, buffer, sizeof(buffer)-1);
        if(n <= 0) break;
        if(total + n < RESPONSE_SIZE - 1) {
            memcpy(task->response + total, buffer, n);
            total += n;
        }
        memset(buffer, 0, sizeof(buffer));
    }
    task->response[total] = '\0';
    close(sockfd);
    pthread_exit(NULL);
}

/**
 * Creates a truncated view of agent response
 * Limits output to first 3 lines for readability
 * 
 * NOTE: Uses static buffer - not thread safe
 * TODO: Consider implementing scrollable output for long responses
 */
char *summarize_response(const char *response) {
    static char summary[512];
    memset(summary, 0, sizeof(summary));
    const char *start = response;
    int lines = 0, i = 0;
    while(*start && lines < 3 && i < 511) {
        summary[i++] = *start;
        if(*start == '\n') lines++;
        start++;
    }
    summary[i] = '\0';
    return summary;
}

/* UI Components */
void display_ascii() {
    /* ASCII art banner for HawkEye */
    printf("                      _                    \n");
    printf("  /\\  /\\__ ___      _| | _____ _   _  ___  \n");
    printf(" / /_/ / _` \\ \\ /\\ / / |/ / _ \\ | | |/ _ \\ \n");
    printf("/ __  / (_| |\\ V  V /|   <  __/ |_| |  __/ \n");
    printf("\\/ /_/ \\__,_| \\_/\\_/ |_|\\_\\___|\\__, |\\___| \n");
    printf("                               |___/       \n");
    printf("                HawkEye                     \n");
}

void display_menu() {
    printf("Commands:\n");
    printf(" help           : Show help\n");
    printf(" list           : List loaded agents\n");
    printf(" send <command> : Broadcast an abstract scan command to all agents\n");
    printf("                  (e.g., 'SCAN 192.168.1.10 full' or 'POLL 192.168.1.10 80 10 quick')\n");
    printf(" exit           : Exit HawkEye\n");
}

/* 
 * Main orchestrator loop
 * Implements a simple command interpreter with broadcast capability
 * 
 * TODO: Implement command history
 * TODO: Add support for targeting specific agents
 * TODO: Implement configuration reload mechanism
 */
int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <agents.conf>\n", argv[0]);
        exit(1);
    }

    load_agents(argv[1]);
    display_ascii();
    display_menu();

    char input[512];
    while(1) {
        printf("HawkEye> ");
        if(fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;

        if(strcmp(input, "help") == 0) {
            display_menu();
        } else if(strcmp(input, "list") == 0) {
            for(int i = 0; i < agent_count; i++) {
                printf("%d: %s %d\n", i+1, agents[i].ip, agents[i].port);
            }
        } else if(strncmp(input, "send ", 5) == 0) {
            char *cmd = input + 5;
            pthread_t threads[MAX_AGENTS];
            AgentTask tasks[MAX_AGENTS];

            /* Parallel command execution */
            for(int i = 0; i < agent_count; i++) {
                tasks[i].agent = agents[i];
                strncpy(tasks[i].command, cmd, sizeof(tasks[i].command)-1);
                tasks[i].response[0] = '\0';
                pthread_create(&threads[i], NULL, send_command, &tasks[i]);
            }

            /* Wait for all responses */
            for(int i = 0; i < agent_count; i++) {
                pthread_join(threads[i], NULL);
            }

            /* Display results */
            printf("---- Summary ----\n");
            for(int i = 0; i < agent_count; i++) {
                printf("Agent %s:%d summary:\n%s\n", 
                       tasks[i].agent.ip, tasks[i].agent.port, 
                       summarize_response(tasks[i].response));
            }
            printf("---- End of Summary ----\n");
        } else if(strcmp(input, "exit") == 0) {
            break;
        } else {
            printf("Unknown command. Type 'help' for menu.\n");
        }
    }
    return 0;
}