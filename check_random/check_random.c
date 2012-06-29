#include <stdio.h>

int main(int argc, char **argv)
{
	printf("check_random | test=1\n");

	srand(getpid());

	return rand() % 4;
}
