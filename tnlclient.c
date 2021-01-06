#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_MAX 65535

char *tnl_ssh_cmd(const char *host, const int port, const char *cmd) {
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

int tnl_get_sftp(const char *host, const int port, const char *src, const char *dest){
    char buffer[BUFFER_MAX];
    char *command;
    char *receiveMessage = calloc(1, sizeof(char));
    int sockfd = 0, rc = 0, total_of_bytes = 0;
    FILE *fp;

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
    asprintf(&command, "g %s", src);
    printf("send msg '%s'\n", command);
    send(sockfd, command, strlen(command),0);
    free(command);

    if((fp = fopen(dest, "wb")) == NULL){
        perror("fopen error\n");
        exit(1);
    }

    while((rc = recv(sockfd, buffer, sizeof(buffer), 0)) > 0){
        total_of_bytes += rc;
        fwrite(buffer, sizeof(char), rc, fp);
        memset(buffer,'\0', sizeof(buffer));      
    }
    printf("close file\n");
    fclose(fp);
    printf("close Socket\n");
    close(sockfd);
    return total_of_bytes;
}

int tnl_put_sftp(const char *host, const int port, const char *src, const char *dest){
    char buffer[BUFFER_MAX];
    char *receiveMessage = calloc(1, sizeof(char));
    int sockfd = 0, rc = 0, total_of_bytes = 0, nread = 0;
    FILE *fp;

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

    if((fp = fopen(src, "rb")) == NULL){
        perror("fopen error\n");
        exit(1);
    }
    // Sending command
    // memset(buffer,'\0', sizeof(buffer));
    // buffer[0] = 'p';
    // buffer[1] = ' ';
    // memcpy(buffer+2, dest, strlen(dest));
    // asprintf(&buffer, "p %s ", dest); // TODO: asprintf cannnot use with static array
    // printf("send msg '%s'\n", buffer);
    // send(sockfd, buffer, sizeof(buffer),0);

    snprintf(buffer,sizeof(buffer),"p %s",dest); 
    // send(sockfd, "p /tmp/README2020.md", 26,0);
    send(sockfd, buffer, 26,0);
    memset(buffer,'\0', sizeof(buffer));
    
    
    while((rc = recv(sockfd, buffer, sizeof(buffer), 0)) > 0){
        // total_of_bytes += rc;
        // fwrite(buffer, sizeof(char), rc, fp);
        // memset(buffer,'\0', sizeof(buffer));      
        if(strstr(buffer,"start")){
            printf("start\n");
            break;
        }
        
    }

    while(!feof(fp)){
        nread = fread(buffer, sizeof(char), sizeof(buffer), fp);
        
        // printf("%s\n",buffer);
        send(sockfd, buffer, nread,0);
        memset(buffer,'\0', sizeof(buffer));
    }

    printf("close file\n");
    fclose(fp);
    printf("close Socket\n");
    close(sockfd);
    return total_of_bytes;
}