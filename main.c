#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef struct ProcStat 
{
	char name[5];
	long int user;
	long int nice;
	long int system;
	long int idle;
	long int iowait;
	long int irq;
	long int softirc;
} CPUstat;

CPUstat** ArrayOfData;

void initialArray(int nCore)
{
	assert(nCore > 0);
	int counter = 0;
	ArrayOfData = (CPUstat **)malloc(nCore * sizeof(CPUstat *));
	if (ArrayOfData != NULL)
	{
		while(counter < nCore)
		{
			*(ArrayOfData+counter) = (CPUstat*)malloc(2 * sizeof(CPUstat));
			if (*(ArrayOfData+counter) == NULL)
			{
				while (counter > 0)
				{	
					free(*(ArrayOfData+counter));
					counter--;
				}
			}
			counter++;
		}
	}
	else
	{
		free(ArrayOfData);
	}
}

void freeArray(int nCore)
{
	assert(nCore > 0);
	int counter = 0;
	
	while(counter < nCore)
	{
		free(ArrayOfData[counter]);
		counter++;
	}
	
	free(ArrayOfData);
}
	


int main(int argc, char* argv[])
{
	initialArray(4);
	freeArray(4);
	return 0;
}
