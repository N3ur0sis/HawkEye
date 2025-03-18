#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define CMD_BUFFER_SIZE 1024
#define OUTPUT_BUFFER_SIZE 1024
void execute_and_send(const char *cmd, int client_fd) {
    FILE *fp = popen(cmd, "r");
    if(!fp){ char err[]="Error executing command.\n"; write(client_fd, err, strlen(err)); return; }
    char output[OUTPUT_BUFFER_SIZE]; memset(output, 0, sizeof(output));
    while(fgets(output, sizeof(output), fp) != NULL){ write(client_fd, output, strlen(output)); memset(output, 0, sizeof(output)); }
    pclose(fp);
}
int main(int argc, char *argv[]){
    if(argc < 2){ fprintf(stderr,"Usage: %s <PORT>\n", argv[0]); return 1; }
    int port = atoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){ perror("socket()"); exit(EXIT_FAILURE); }
    struct sockaddr_in serv_addr; memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; serv_addr.sin_port = htons(port); serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){ perror("bind()"); close(server_fd); exit(EXIT_FAILURE); }
    if(listen(server_fd, 5) < 0){ perror("listen()"); close(server_fd); exit(EXIT_FAILURE); }
    struct sockaddr_in client_addr; socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if(client_fd < 0){ perror("accept()"); close(server_fd); exit(EXIT_FAILURE); }
    char command[CMD_BUFFER_SIZE]; memset(command, 0, sizeof(command));
    ssize_t n = read(client_fd, command, sizeof(command)-1);
    if(n <= 0){ perror("read()"); close(client_fd); close(server_fd); exit(EXIT_FAILURE); }
    command[n] = '\0';
    char *token = strtok(command, " \t\n");
    if(token == NULL){ char msg[]="Empty command.\n"; write(client_fd, msg, strlen(msg)); close(client_fd); close(server_fd); exit(EXIT_FAILURE); }
    if(strcmp(token, "SCAN") == 0){
        char *target = strtok(NULL, " \t\n");
        char *mode = strtok(NULL, " \t\n");
        if(target==NULL || mode==NULL){ char msg[]="Usage: SCAN <target> <mode>\n"; write(client_fd, msg, strlen(msg)); }
        else {
            char zap_cmd[1024];
            if(strcmp(mode, "full")==0)
                snprintf(zap_cmd, sizeof(zap_cmd), "zap-cli quick-scan %s -r", target);
            else if(strcmp(mode, "quick")==0)
                snprintf(zap_cmd, sizeof(zap_cmd), "zap-cli quick-scan %s", target);
            else
                snprintf(zap_cmd, sizeof(zap_cmd), "zap-cli quick-scan %s %s", target, mode);
            execute_and_send(zap_cmd, client_fd);
        }
    } else if(strcmp(token, "POLL") == 0){
        char *target = strtok(NULL, " \t\n");
        char *interval_str = strtok(NULL, " \t\n");
        char *mode = strtok(NULL, " \t\n");
        if(target==NULL || interval_str==NULL || mode==NULL){ char msg[]="Usage: POLL <target> <interval> <mode>\n"; write(client_fd, msg, strlen(msg)); }
        else {
            int interval = atoi(interval_str);
            char zap_cmd[1024];
            if(strcmp(mode, "full")==0)
                snprintf(zap_cmd, sizeof(zap_cmd), "zap-cli quick-scan %s -r", target);
            else if(strcmp(mode, "quick")==0)
                snprintf(zap_cmd, sizeof(zap_cmd), "zap-cli quick-scan %s", target);
            else
                snprintf(zap_cmd, sizeof(zap_cmd), "zap-cli quick-scan %s %s", target, mode);
            for(int i=0;i<3;i++){ execute_and_send(zap_cmd, client_fd); sleep(interval); }
        }
    } else { char msg[]="Unknown command. Use SCAN or POLL.\n"; write(client_fd, msg, strlen(msg)); }
    close(client_fd); close(server_fd);
    return 0;
}