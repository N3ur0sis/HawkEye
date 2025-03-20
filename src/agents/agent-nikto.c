#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define CMD_BUFFER_SIZE 1024
#define OUTPUT_BUFFER_SIZE 1024

/* 
 * Execute shell command and stream output to client socket
 * TODO: Consider implementing timeout mechanism for long-running commands
 * TODO: Add error handling for buffer overflow scenarios
 */
void execute_and_send(const char *cmd, int client_fd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        char err[] = "Error executing command.\n";
        write(client_fd, err, strlen(err));
        return;
    }

    char output[OUTPUT_BUFFER_SIZE];
    memset(output, 0, sizeof(output));
    
    // Stream output line by line to avoid memory buffer issues
    while (fgets(output, sizeof(output), fp) != NULL) {
        write(client_fd, output, strlen(output));
        memset(output, 0, sizeof(output));
    }
    pclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    // Network setup phase
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

    if (listen(server_fd, 5) < 0) {
        perror("listen()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Accept single client connection
    // NOTE: Currently handles only one client at a time
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Command processing phase
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

    // Parse command using space/tab/newline as delimiters
    char *token = strtok(command, " \t\n");
    if (token == NULL) {
        char msg[] = "Empty command.\n";
        write(client_fd, msg, strlen(msg));
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* Command handler section
     * Supports two main commands:
     * 1. SCAN - Single nikto scan
     * 2. POLL - Repeated nikto scan with interval
     * TODO: Consider implementing command handler as separate functions
     * TODO: Add support for scan interruption
     */
    if (strcmp(token, "SCAN") == 0) {
        char *target = strtok(NULL, " \t\n");
        char *mode = strtok(NULL, " \t\n");
        
        if (target == NULL || mode == NULL) {
            char msg[] = "Usage: SCAN <target> <mode>\n";
            write(client_fd, msg, strlen(msg));
        } else {
            char nikto_cmd[1024];
            // Configure nikto command based on mode
            if (strcmp(mode, "full") == 0)
                snprintf(nikto_cmd, sizeof(nikto_cmd), "nikto -h %s -Display V", target);
            else if (strcmp(mode, "quick") == 0)
                snprintf(nikto_cmd, sizeof(nikto_cmd), "nikto -h %s -Tuning 9", target);
            else
                snprintf(nikto_cmd, sizeof(nikto_cmd), "nikto -h %s %s", target, mode);
            execute_and_send(nikto_cmd, client_fd);
        }
    } else if (strcmp(token, "POLL") == 0) {
        char *target = strtok(NULL, " \t\n");
        char *port_str = strtok(NULL, " \t\n");
        char *interval_str = strtok(NULL, " \t\n");
        char *mode = strtok(NULL, " \t\n");
        
        if (target == NULL || port_str == NULL || interval_str == NULL || mode == NULL) {
            char msg[] = "Usage: POLL <target> <port> <interval> <mode>\n";
            write(client_fd, msg, strlen(msg));
        } else {
            int interval = atoi(interval_str);
            char nikto_cmd[1024];
            // Configure nikto command for polling
            if (strcmp(mode, "full") == 0)
                snprintf(nikto_cmd, sizeof(nikto_cmd), "nikto -h %s -p %s -Display V", target, port_str);
            else if (strcmp(mode, "quick") == 0)
                snprintf(nikto_cmd, sizeof(nikto_cmd), "nikto -h %s -p %s -Tuning 9", target, port_str);
            else
                snprintf(nikto_cmd, sizeof(nikto_cmd), "nikto -h %s -p %s %s", target, port_str, mode);
            
            // Execute scan 3 times with specified interval
            // TODO: Make number of iterations configurable
            for (int i = 0; i < 3; i++) {
                execute_and_send(nikto_cmd, client_fd);
                sleep(interval);
            }
        }
    } else {
        char msg[] = "Unknown command. Use SCAN or POLL.\n";
        write(client_fd, msg, strlen(msg));
    }

    close(client_fd);
    close(server_fd);
    return 0;
}