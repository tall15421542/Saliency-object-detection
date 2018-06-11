#include <stdio.h>
int main(int argc, char *argv[])
{
	fprintf(stderr, "%s", argv[1]);
	fopen("OK", "w");
	return 0;
}
