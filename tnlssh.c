#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
 
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

int sftp_get_file(ssh_session session, sftp_session sftp, const char* file_src){
  int access_type;
  sftp_file file;
  char buffer[BUFFER_MAX];
  int nbytes, nwritten, rc;
  FILE *fp;
 
  //access_type = O_RDONLY;
  file = sftp_open(sftp, file_src, access_type, 0); // read the file by sftp
  if (file == NULL) {
      fprintf(stderr, "Can't open file for reading: %s\n", ssh_get_error(session));
      return SSH_ERROR;
  }
 
  fp = fopen("tmp.txt", "w");
  if (fp < 0) {
      fprintf(stderr, "Can't open file for writing: %s\n", strerror(errno));
      return SSH_ERROR;
  }
 
  for (;;) {
      // read through sftp
      nbytes = sftp_read(file, buffer, sizeof(buffer));

      if (nbytes == 0) {
          break; // EOF
      } else if (nbytes < 0) {
          fprintf(stderr, "Error while reading file: %s\n", ssh_get_error(session));
          sftp_close(file);
          return SSH_ERROR;
      }

      // write normally
      nwritten = fwrite(buffer, 1, nbytes, fp); // write to the temporary file
      if (nwritten != nbytes) {
          fprintf(stderr, "Error writing: %s\n", strerror(errno));
          sftp_close(file);
          return SSH_ERROR;
      }
  }

  printf("get file success\n");
 
  fclose(fp);
  rc = sftp_close(file);
  if (rc != SSH_OK) {
      fprintf(stderr, "Can't close the read file: %s\n", ssh_get_error(session));
      return rc;
  }
  return rc;
}

// int sftp_put_file(int clientSockfd,ssh_session session, sftp_session sftp, const char* file_dest, FILE *fp){
int sftp_put_file(int clientSockfd, ssh_session session, sftp_session sftp, const char* file_dest){
  
    

  // int access_type ;
  int access_type = O_WRONLY|O_CREAT|O_TRUNC;
  // int access_type = O_WRONLY|O_CREAT;
  sftp_file file;
  char buffer[BUFFER_MAX];
  int nbytes, nread, rc;
  file = sftp_open(sftp, file_dest, access_type, S_IRWXU); // read the file by sftp
  if (file == NULL) {
      // fprintf(stderr, "Can't open file for reading: %s\n", sftp_get_error(sftp));
      return SSH_ERROR;
  }
  
  // ask client send file
  char* start_key = "start";
  send(clientSockfd, start_key, strlen(start_key),0);

  char inputBuffer[256];
  // read the fp as long as it has remaining data
  // while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
  while ((nread = recv(clientSockfd, inputBuffer, sizeof(inputBuffer), 0)) > 0) {
      // write through sftp
      if(nread!=sizeof(inputBuffer)) {inputBuffer[nread]='\0';}
 
      if (nread != sftp_write(file, inputBuffer, nread)){
      // if (nread != sftp_write(file, inputBuffer, sizeof(inputBuffer))){
        fprintf(stderr, "Error while reading file: %s\n", ssh_get_error(session));
        sftp_close(file);
        return SSH_ERROR;
      }
  }

  printf("put file success\n");
  rc = sftp_close(file);
  if (rc != SSH_OK) {
      fprintf(stderr, "Can't close the read file: %s\n", ssh_get_error(session));
      return rc;
  }
  return rc;
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

  // For sftp
  sftp_session my_sftp;
  int sftp_rc;
  my_sftp = sftp_new(my_ssh_session);
  // Check allocation of sftp
  if (my_sftp == NULL) {
    fprintf(stderr, "Error allocating SFTP session: %s\n", ssh_get_error(my_sftp));
    return SSH_ERROR;  
  }
  // Check sftp session
  sftp_rc = sftp_init(my_sftp);
  if (sftp_rc != SSH_OK) {
    fprintf(stderr, "Error initializing SFTP session: code %d.\n", sftp_get_error(my_sftp));
    sftp_free(my_sftp);
    return sftp_rc;
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
    int nrecv;
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
        printf("receive cmd '%s'\n", inputBuffer); // TODO: a recv maybe not only get one send's content
        
        strtok(inputBuffer, " ");
        if (inputBuffer[0] == 'g'){ // in the ftp get command
          char *tmp_src = strtok(NULL, " "); // get the src file name
          if (sftp_get_file(my_ssh_session, my_sftp, tmp_src) == SSH_OK){
            int fd = open("tmp.txt", O_RDONLY); // open the tmp file
            struct stat s; // set up fd's stat
            fstat(fd, &s);
            char *adr = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0); // mmap the file from the fd
            write(clientSockfd, adr, s.st_size); // send a buffer pointed by adr to fd
            close(fd);
          } else {
            fprintf(stderr, "Socket file transfer failure\n");
            break;
          }
        } else if (inputBuffer[0] == 'p') { // in the ftp put command
          char *tmp_src = strtok(NULL, " "); // get the path where to store the file
          // sftp_put_file(my_ssh_session, my_sftp, tmp_src)
          if (sftp_put_file(clientSockfd, my_ssh_session, my_sftp, tmp_src) == SSH_OK){

          } else {
            fprintf(stderr, "Socket file transfer failure\n");
            break;
          }

          
          
          // fclose(fp);
          // printf("copy file complete\n");
        } else {
          if (execute_remote_cmd(my_ssh_session, inputBuffer, &retval) != SSH_OK){
            fprintf(stderr, "SSH failure\n");
            break;
          }
          printf("get return value :\n%s", retval);
          send(clientSockfd, retval, strlen(retval), 0);
          // Clean memory
          free(retval);
        }

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
  sftp_free(my_sftp);
}
