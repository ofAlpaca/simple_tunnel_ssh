#include <stdio.h>
#include <stdlib.h>
#include "tnlclient.c"

/*
You can make a command such as "./example <command>" to show the return value.
for example, ./example "whoami" will return "amas".
*/

int main(int argc , char *argv[]) {
    if (argc != 2)
        fprintf(stderr,"No argument\n");
    
    char *retval = sendSshCmd("127.0.0.1", 8700, argv[1]);
    printf("%s", retval);
    // remember to free the return value !
    free(retval);
    return 0;
}
