#include <stdio.h>
void test(char** a)
{
	*a=NULL;

}
void main()
{
	char* b="123";
	test(&b);
	printf("%s\n",b);
}
