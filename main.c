#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct ProcStat 
{
	char name[5];		// Name of CPU
	long int* member;	// Array of data for one CPU 
	int nMember;		// Count of data for one CPU
	float usage;		// Procent usage of CPU
} CPUstat;

CPUstat** ArrayOfData;

void initialArray(int nCore, int nMember)
{
	//assert(nCore > 0);
	int counter_1 = 0;
	int counter_2 = 0;
	
	ArrayOfData = (CPUstat **)calloc(2, sizeof(CPUstat *));
	if (ArrayOfData != NULL){
		
		while(counter_1 < 2){
			*(ArrayOfData+counter_1) = (CPUstat*)calloc(nCore ,sizeof(CPUstat));
			
			if (*(ArrayOfData+counter_1) == NULL ){
				
				while (counter_1 > 0){	
					free((*(ArrayOfData+counter_1))->member );
					free(*(ArrayOfData+counter_1));
					counter_1--;
				}
			}
			else{
				
				while(counter_2 < nCore){
					(*(ArrayOfData+counter_1)+counter_2)->nMember = nMember;
					(*(ArrayOfData+counter_1)+counter_2)->member = (long int*)calloc((*(ArrayOfData+counter_1)+counter_2)->nMember, sizeof(long int));
					
					if ((*(ArrayOfData+counter_1)+counter_2)->member == NULL) {
						free(	((*(ArrayOfData + counter_1))+counter_2)	);
					}
					counter_2++;
				}
			}
			counter_1++;
			counter_2=0;
		}
	}
	else
	{
		free(ArrayOfData);
	}
}

void freeArray(int nCore)
{
	//assert(nCore > 0);
	int counter_1 = 0;
	int counter_2 = 0;
	
	while(counter_1 < 2)
	{
		while(counter_2 < nCore) {
			free(ArrayOfData[counter_1][counter_2].member);
			counter_2++;
		}
		free(ArrayOfData[counter_1]);
		counter_1++;
		counter_2 = 0;
	}
	free(ArrayOfData);
}

void checkData(int* nCore, int* nMember) {
	FILE* data = NULL;
	char line[200];
	int counter = 0;
	
	//assert(*nMember == 0);
	//assert(*nCore == 0);
	
	data = fopen("/proc/stat", "r");
	//assert(data!=NULL);
	
	fgets(line, 200, data);
	while (line[counter] != '\0')
	{
		if ((line[counter]) == ' ')
		{
			(*nMember)++;
		}
		counter++;
	}
	(*nMember)--;
	
	do{
		fgets(line, 200, data);
		(*nCore)++;
	}while((strtok(line, " "))[0] == 'c'); // c is the first char of CPU
	(*nCore)--;
	
}

void checkDataTest() {
	int a = 0;
	int b = 0;
	checkData(&a, &b);
	//assert(a > 0 && b > 0);
	//assert(a == 4);			//spec value for my system
	//assert(b == 10); 		//spec value for my system
}

void getData(int nCore) {
	FILE* data;
	char line[200];
	int counter_1 = 0;
	int counter_2 = 0;
	
	data = fopen("/proc/stat", "r");
	//assert(data != NULL);
	
	fgets(line, 200, data);
	
	while(counter_1 < nCore) {
		fscanf(data,"%s", ArrayOfData[0][counter_1].name);
		while(counter_2 < ArrayOfData[0][counter_1].nMember) {
			fscanf(data,"%ld", ArrayOfData[0][counter_1].member+counter_2);
			counter_2++;
		}
		counter_1++;
		counter_2 = 0;
	}
	fclose(data);
}

void printCPUstat(int nCore) {
	int counter_1 = 0;
	int counter_2 = 0;
	
	//assert(nCore > 0);
	
	while(counter_1 < nCore) {
		printf("%s ", ArrayOfData[0][counter_1].name);
		while(counter_2 < ArrayOfData[0][counter_1].nMember) {
			printf("%ld ", ArrayOfData[0][counter_1].member[counter_2]);
			counter_2++;
		}
		puts("");
		counter_1++;
		counter_2 = 0;
	}
}

int main(int argc, char* argv[]) {
	int nCore;
	int nMember;
	checkData(&nCore, &nMember);
	initialArray(nCore, nMember);
	getData(nCore);
	printCPUstat(nCore);
	//checkDataTest();
	freeArray(nCore);
	return 0;
}
