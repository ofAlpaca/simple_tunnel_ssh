#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
void test(char** a)
{
	*a=NULL;

}
void main()
{
  char *str = "Hi ABCD";
  int fd = open("hi.md",O_CREAT|O_WRONLY);
  write(fd, str, strlen(str));
  close(fd);
}
