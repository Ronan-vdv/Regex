#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	//Program name is the first arg
	if (argc <= 1)
	{
		printf("No arguments provided\n");
		return 0;
	}

	//printf("%s\n", argv[1]);
	//printf("%i\n", strcmp(argv[1], "-f"));

	//Get input from file
	if (!strcmp(argv[1], "-f"))
	{
		if (argc < 3)
		{
			printf("No file provided\n");
			return 0;
		}
		FILE* file = fopen(argv[2], "r");
		if (!file)
		{
			printf("File could not be opened\n");
			return 0;
		}
		char c = fgetc(file);

		while (1)
		{
			if (feof(file))
				break;

			printf("%c", c);
			c = fgetc(file);
		}
	}
	else
	{
		int ccount = 0;
		char c = argv[1][0];
		while (c != '\0')
		{
			printf("%c", c);

			c = argv[1][++ccount];
		}
	}

	return 0;
}
