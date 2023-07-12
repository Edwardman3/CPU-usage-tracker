#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct ProcStat 
{
	char name[5];									// Name of CPU
	unsigned long long int* member;					// Array of data for one CPU 
	int nMember;									// Count of data for one CPU
	float usage;					// Procent usage of CPU
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
					(*(ArrayOfData+counter_1)+counter_2)->member = (unsigned long long int*)calloc((*(ArrayOfData+counter_1)+counter_2)->nMember, sizeof(unsigned long long int));
					
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

void makeDataOld()
{
	CPUstat* temp = ArrayOfData[0];
	ArrayOfData[0] = ArrayOfData[1];
	ArrayOfData[1] = temp;
}

void getData(int nCore) {
	FILE* data;
	char line[200];
	int counter_1 = 0;
	int counter_2 = 0;
	
	
	data = fopen("/proc/stat", "r");
	
	assert(data != NULL);
	
	makeDataOld();
	
	fgets(line, 200, data);
	
	while(counter_1 < nCore) {
		fscanf(data,"%s", ArrayOfData[0][counter_1].name);
		while(counter_2 < ArrayOfData[0][counter_1].nMember) {
			fscanf(data,"%llu", ArrayOfData[0][counter_1].member+counter_2);
			counter_2++;
		}
		counter_1++;
		counter_2 = 0;
	}
	fclose(data);
}

void printCPUstat(int nCore, int dtype) //nCore - number of core, dtype - data type 1 - old 0 - new;
{
	int counter_1 = 0;
	int counter_2 = 0;
	
	//assert(nCore > 0);
	
	while(counter_1 < nCore) {
		printf("%s ", ArrayOfData[dtype][counter_1].name);
		while(counter_2 < ArrayOfData[dtype][counter_1].nMember) {
			printf("%llu ", ArrayOfData[dtype][counter_1].member[counter_2]);
			counter_2++;
		}
		puts("");
		counter_1++;
		counter_2 = 0;
	}
}


/*
	0	|1	 |2		|3	 |4		|5	|6		|7	  |8	|9 
	user|nice|system|idle|iowait|irq|softirq|steal|guest|guestnice
	 
	0 - new data
	1 - old data
*/



void calcCPUusage(int nCore)
{
	unsigned long long int PrevIdle 	= 0;
	unsigned long long int Idle 		= 0;
	unsigned long long int PrevNonIdle 	= 0;
	unsigned long long int NonIdle 		= 0;
	unsigned long long int PrevTotal 	= 0;
	unsigned long long int Total 		= 0;
	unsigned long long int totald 		= 0;
	unsigned long long int idled 		= 0;
	
	for(int i = 0;i < nCore ;i++)
	{
		PrevIdle 	= ArrayOfData[1][i].member[3] + ArrayOfData[1][i].member[4];
		Idle 		= ArrayOfData[0][i].member[3] + ArrayOfData[0][i].member[4];
		
		PrevNonIdle = ArrayOfData[1][i].member[0] + ArrayOfData[1][i].member[1] + \
					  ArrayOfData[1][i].member[2] + ArrayOfData[1][i].member[5] + \
					  ArrayOfData[1][i].member[6] + ArrayOfData[1][i].member[7];
					  
		NonIdle 	= ArrayOfData[0][i].member[0] + ArrayOfData[0][i].member[1] + \
					  ArrayOfData[0][i].member[2] + ArrayOfData[0][i].member[5] + \
					  ArrayOfData[0][i].member[6] + ArrayOfData[0][i].member[7];
	
		PrevTotal 	= PrevIdle 	+ PrevNonIdle;
		Total 		= Idle 		+ NonIdle;
		totald 		= Total 	- PrevTotal;
		idled 		= Idle 		- PrevIdle;
		
		
		assert(totald > 0);
		
		ArrayOfData[0][i].usage = 100*(totald - idled)/(double)totald;
	}
}

int main(int argc, char* argv[]) {
	int testIteration = 100;
	int nCore;
	int nMember;
	checkData(&nCore, &nMember);
	initialArray(nCore, nMember);
	getData(nCore);
	//printCPUstat(nCore,0);
	while(testIteration>0)
	{
		sleep(1);
		getData(nCore);
		calcCPUusage(nCore);
		system("clear");
		printf("%s %5.2lf \n", ArrayOfData[0][0].name, ArrayOfData[0][0].usage);
		printf("%s %5.2lf \n", ArrayOfData[0][1].name, ArrayOfData[0][1].usage);
		printf("%s %5.2lf \n", ArrayOfData[0][2].name, ArrayOfData[0][2].usage);
		printf("%s %5.2lf \n", ArrayOfData[0][3].name, ArrayOfData[0][3].usage);
		//printCPUstat(nCore,1);
		//printCPUstat(nCore,0);
		//checkDataTest();
		testIteration--;
}
	freeArray(nCore);
	return 0;
}
