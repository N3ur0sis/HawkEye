#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define CMD_BUFFER_SIZE 1024
#define OUTPUT_BUFFER_SIZE 1024

// Helper function to execute shell commands and stream output to client
// TODO: Consider implementing timeout mechanism for long-running commands
void execute_and_send(const char *cmd, int client_fd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        char err[] = "Error executing command.\n";
        write(client_fd, err, strlen(err));
        return;
    }

    char output[OUTPUT_BUFFER_SIZE];
    memset(output, 0, sizeof(output));
    
    // Stream output buffer-by-buffer to client
    while (fgets(output, sizeof(output), fp) != NULL) {
        write(client_fd, output, strlen(output));
        memset(output, 0, sizeof(output));
    }
    
    pclose(fp);
}

int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    // Network setup
    int port = atoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // TODO: Consider adding SO_REUSEADDR socket option
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // TODO: Make backlog configurable
    if (listen(server_fd, 5) < 0) {
        perror("listen()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Accept single client connection
    // NOTE: Currently handles only one client and exits
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Command processing
    char command[CMD_BUFFER_SIZE];
    memset(command, 0, sizeof(command));
    ssize_t n = read(client_fd, command, sizeof(command)-1);
    if (n <= 0) {
        perror("read()");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    command[n] = '\0';

    // Parse space-delimited command string
    // TODO: Implement more robust command parsing
    char *token = strtok(command, " \t\n");
    if (token == NULL) {
        char msg[] = "Empty command.\n";
        write(client_fd, msg, strlen(msg));
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Command handlers
    if (strcmp(token, "SCAN") == 0) {
        // SCAN command implementation
        // Format: SCAN <target> <mode>
        char *target = strtok(NULL, " \t\n");
        char *mode = strtok(NULL, " \t\n");
        
        if (target == NULL || mode == NULL) {
            char msg[] = "Usage: SCAN <target> <mode>\n";
            write(client_fd, msg, strlen(msg));
        } else {
            // TODO: Implement input validation for target
            char nmap_cmd[1024];
            if (strcmp(mode, "full") == 0)
                snprintf(nmap_cmd, sizeof(nmap_cmd), "nmap %s -sV -p1-65535", target);
            else if (strcmp(mode, "quick") == 0)
                snprintf(nmap_cmd, sizeof(nmap_cmd), "nmap %s -T4 -F", target);
            else
                snprintf(nmap_cmd, sizeof(nmap_cmd), "nmap %s %s", target, mode);
            
            execute_and_send(nmap_cmd, client_fd);
        }
    } else if (strcmp(token, "POLL") == 0) {
        // POLL command implementation
        // Format: POLL <target> <port> <interval> <mode>
        char *target = strtok(NULL, " \t\n");
        char *port_str = strtok(NULL, " \t\n");
        char *interval_str = strtok(NULL, " \t\n");
        char *mode = strtok(NULL, " \t\n");

        if (target == NULL || port_str == NULL || interval_str == NULL || mode == NULL) {
            char msg[] = "Usage: POLL <target> <port> <interval> <mode>\n";
            write(client_fd, msg, strlen(msg));
        } else {
            // TODO: Add bounds checking for interval
            int interval = atoi(interval_str);
            char nmap_cmd[1024];
            
            if (strcmp(mode, "full") == 0)
                snprintf(nmap_cmd, sizeof(nmap_cmd), "nmap -p %s %s -sV", port_str, target);
            else if (strcmp(mode, "quick") == 0)
                snprintf(nmap_cmd, sizeof(nmap_cmd), "nmap -p %s %s -T4", port_str, target);
            else
                snprintf(nmap_cmd, sizeof(nmap_cmd), "nmap -p %s %s %s", port_str, target, mode);
            
            // Fixed number of polls - could be made configurable
            for (int i = 0; i < 3; i++) {
                execute_and_send(nmap_cmd, client_fd);
                sleep(interval);
            }
        }
    } else {
        char msg[] = "Unknown command. Use SCAN or POLL.\n";
        write(client_fd, msg, strlen(msg));
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    return 0;
}