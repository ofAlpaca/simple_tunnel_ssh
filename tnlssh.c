#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
 
#define BUFFER_MAX 65535
#define LISTENQ 1024
#define TIMEOUT 600 // 10 minutes

int execute_remote_cmd(ssh_session session, const char *cmd, char **retval)
{
  ssh_channel channel;
  int rc;
  char buffer[BUFFER_MAX];
  int nbytes;
  *retval = calloc(1,sizeof(char));

  channel = ssh_channel_new(session);
  if (channel == NULL)
    return SSH_ERROR;

  rc = ssh_channel_open_session(channel);

  if (rc != SSH_OK)
  {
    ssh_channel_free(channel);
    return rc;
  }
  
  rc = ssh_channel_request_exec(channel, cmd); // Executing command
  
  if (rc != SSH_OK)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return rc;
  }

  // Concating the return value from ssh server
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0){
    *retval = realloc(*retval, strlen(*retval) + nbytes);
    strncat(*retval, buffer, nbytes);
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);

    if (nbytes < 0) {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return SSH_ERROR;
    }
  }

  printf("%d\n", strlen(*retval));
  // Closing the channel
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
 
  return SSH_OK;
}

int main(int argc, char *argv[]) {
  if (argc != 5){
    fprintf(stderr, "Command should be like \"tnlssh <password> <user_name> <hostname/ip> <port>\"\n");
    exit(-1);
  }

  // For ssh
  ssh_session my_ssh_session;
  int rc;
  char *password;

  // For socket server
  char inputBuffer[256];
  char *retval;
  int sockfd = 0, clientSockfd = 0;
 
  // Open session and set options
  my_ssh_session = ssh_new();
  if (my_ssh_session == NULL)
    exit(-1);

  ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, argv[3]);
  ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, argv[2]);
  password = argv[1];

  // Connect to ssh server
  rc = ssh_connect(my_ssh_session);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error connecting to localhost: %s\n",
            ssh_get_error(my_ssh_session));
    ssh_free(my_ssh_session);
    exit(-1);
  }
 
  // Authenticate ourselves
  rc = ssh_userauth_password(my_ssh_session, NULL, password);
  if (rc != SSH_AUTH_SUCCESS)
  {
    fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(my_ssh_session));
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    exit(-1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1){
    fprintf(stderr,"Socket error\n","");
    exit(-1);
  }

  struct sockaddr_in serverInfo, clientInfo;
  int addrlen = sizeof(clientInfo);
  bzero(&serverInfo,sizeof(serverInfo));

  serverInfo.sin_family = PF_INET;
  serverInfo.sin_addr.s_addr = INADDR_ANY;
  serverInfo.sin_port = htons(strtol(argv[4], NULL, 10));
  bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
  listen(sockfd,LISTENQ);
  //freeaddrinfo(&serverInfo);

  // The blocking time for `select`
  struct timeval tv = {TIMEOUT, 0};

  while(1){
    fd_set fds; // Create a file_describer set
    FD_ZERO(&fds); // Clear all bit in fd_set
    FD_SET(sockfd, &fds); // Add sockfd to set fds

    // `select` will monitor multiple fdsm checking fi they're ready.
    rc = select(sockfd+1, &fds, NULL, NULL, &tv);

    if (rc == -1) // fds not ready
      continue;
    else if (rc > 0){ // fds is ready for reading
      if (FD_ISSET(sockfd, &fds)){
        clientSockfd = accept(sockfd, (struct sockaddr*) &clientInfo, &addrlen);
        recv(clientSockfd, inputBuffer, sizeof(inputBuffer), 0);
        printf("receive cmd '%s'\n", inputBuffer);
        if (execute_remote_cmd(my_ssh_session, inputBuffer, &retval) != SSH_OK){
          fprintf(stderr, "SSH failure\n","");
          break;
        }
        printf("get return value :\n%s", retval);
        send(clientSockfd, retval, strlen(retval), 0);
        // Clean memory
        free(retval);
        memset(inputBuffer, '\0', sizeof(inputBuffer));
	      close(clientSockfd);
        printf("Successul done\n");
      }
    }

    // `ssh_send_ignore will keep the session alive`
    ssh_send_ignore(my_ssh_session, "Polling");

    // After `select` unblock, the timeval need to be reset !
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
  }

  ssh_disconnect(my_ssh_session);
  ssh_free(my_ssh_session);
}
