#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_MAX 256

char *sendSshCmd(const char *host, const int port, const char *cmd) {
    char buffer[BUFFER_MAX];
    char *command;
    char *receiveMessage = calloc(1, sizeof(char));
    int sockfd = 0, rc = 0, n = 1;

    // Socket init
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        fprintf(stderr, "Fail to create a socket.");
        return;
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    // Send from localhost of port 8700
    info.sin_addr.s_addr = inet_addr(host);
    info.sin_port = htons(port);

    // Start to connect
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err == -1){
        fprintf(stderr, "Connection error");
        return;
    }

    // Sending command
    command = strdup(cmd);
    printf("send msg '%s'\n", command);
    send(sockfd, command, strlen(command),0);
    free(command);

    while((rc = recv(sockfd, buffer, sizeof(buffer), 0)) > 0){
        receiveMessage = realloc(receiveMessage, rc + n);
        memcpy(receiveMessage + n - 1, buffer, rc);
        n += rc;
        receiveMessage[n - 1] = '\0';
        memset(buffer,'\0', sizeof(buffer));      
    }

    printf("close Socket\n");
    close(sockfd);
    return receiveMessage;
}
