#include <stdio.h>
#include <stdlib.h>
#include "tnlclient.c"

/*
You can make a command such as "./example <command>" to show the return value.
for example, ./example "whoami" will return "amas".
*/

int main(int argc , char *argv[]) {
    //if (argc != 2)
    //    fprintf(stderr,"No argument\n");
    
    // char *retval = tnl_ssh_cmd("127.0.0.1", 8700, argv[1]);
    // tnl_get_sftp("127.0.0.1", 8700, "/home/IPP.tgz", "./ipp.tgz");
    // tnl_put_sftp("127.0.0.1", 8700, "README.md", "/home/amas/README2020.md");
    tnl_put_sftp("140.116.82.242", 8700, "README.md", "/tmp/README.md");
    // tnl_put_sftp("127.0.0.1", 8700, "README.md", "/tmp/README2020.md");
    // printf("%s", retval);
    // remember to free the return value !
    // free(retval);
    return 0;
}
