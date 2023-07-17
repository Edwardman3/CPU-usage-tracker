#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAX_SIZE_OF_QUEUE 200

volatile sig_atomic_t done = 0;
int nCore = 4;
int nMember = 10;


typedef struct ProcStat 
{
	char name[5];					// Name of CPU
	unsigned long long int* member;	// Array of data for one CPU 
	int nMember;					// Count of data for one CPU
	float usage;					// Procent usage of CPU
} CPUstat;

struct queue {
	struct queue* prevData;				// previous data in a queue if NULL its the last
	CPUstat actualData;				// data in a queue at position
	struct queue* nextData;				// next data in a queue	if NULL its the latest
	int nIter;						// id of the data
};

CPUstat** ArrayOfData;
struct queue* dataReader;

void initQueue(){
	dataReader=(struct queue*)malloc(sizeof(struct queue));
	dataReader->prevData = NULL;
	dataReader->nextData = NULL;
	dataReader->nIter = 1;
	dataReader->actualData.nMember = nMember;
}


void destroyQueue(struct queue *fromData){
	struct queue* tempData;
	struct queue* tempPrevData;
	assert(fromData != NULL);
	if(fromData != NULL){
		tempData=fromData->nextData;
		
		while(fromData != NULL){
			if(fromData->actualData.member != NULL) {
				free(fromData->actualData.member);
			}
			tempPrevData = fromData->prevData;
			free(fromData);
			fromData = tempPrevData;
		}
		if(tempData != NULL){
			tempData->nIter = 1;
			tempData->prevData = NULL;
			tempData = tempData->nextData;
			while (tempData != NULL){
				tempData->nIter = tempData->prevData->nIter + 1;
				dataReader = tempData;
				tempData = tempData->nextData;
			}
		}
	}	
}

void rewindData(int nIter){ // 0 set to the top, 1,2,3.. iter to down, -1, -2,-3,-4... iter to top
	int done = 1;
	int counter = 1;
	while(done){
		if(nIter > 0){
			if((dataReader->prevData != NULL) && (counter <= nIter)){
				dataReader = dataReader->prevData;
			}
			else{
				done = 0;
			}
		}
		else{
			if(nIter < 0){
				if((dataReader->nextData != NULL) && (counter <= (-1)*nIter)){
					dataReader = dataReader->nextData;
				}
				else{
					done = 0;
				}
			}
			else{
				if(dataReader->nextData != NULL){
					dataReader = dataReader->nextData;
				}
				else{
					done = 0;
				}
			}
		}
		counter++;
	}
}


int addDataQueue(){
	FILE* dataFile;
	char line[200];
	int counter_1 = 0;
	int counter_2 = 0;
	
	errno = 0;
	dataFile = fopen("/proc/stat", "r");
	if (errno != 0){
		perror("Error occured. File not open");
		return 1;
	}
	
	fgets(line, 200, dataFile);
	
	//Cleaning old data of Queue
	
	puts("START");
	while(counter_1 < nCore){
		
		if(dataReader == NULL){
			dataReader=(struct queue*)malloc(sizeof(struct queue));
			assert(dataReader != NULL);
			dataReader->prevData = NULL;
			dataReader->nextData = NULL;
			dataReader->nIter = 1;
			dataReader->actualData.nMember = nMember;
		}
		else{
			if (dataReader->nIter == MAX_SIZE_OF_QUEUE){
				rewindData(MAX_SIZE_OF_QUEUE - 4);
				printf("%d \n", dataReader->nIter);
				destroyQueue(dataReader);
				rewindData(0);
				printf("%d \n", dataReader->nIter);
			}
			dataReader->nextData = malloc(sizeof(struct queue));
			assert(dataReader->nextData != NULL);
			dataReader->nextData->prevData = dataReader;
			dataReader = dataReader->nextData; //jump to next member
			dataReader->nextData = NULL;
			dataReader->actualData.nMember = nMember;
			dataReader->nIter = dataReader->prevData->nIter + 1;
		}
		
		fscanf(dataFile,"%s", dataReader->actualData.name);
		dataReader->actualData.member = (unsigned long long int*)malloc(nMember * sizeof(unsigned long long int));
		
		while(counter_2 < nMember){
			fscanf(dataFile,"%llu", &dataReader->actualData.member[counter_2]);
			counter_2++;
		}		
		counter_2=0;
		counter_1++;
	}
	fclose(dataFile);
	return 0;
}
	
void printStat(){
	int counter_1 = 0;
	int counter_2 = 0;
	struct queue* tempReader = dataReader;
	assert(nCore > 0);
	assert(tempReader != NULL);
	while(counter_1 < nCore) {
		printf("%3d %s ", tempReader->nIter, tempReader->actualData.name);
		while(counter_2 < tempReader->actualData.nMember) {
			printf("%llu ", tempReader->actualData.member[counter_2]);
			counter_2++;
		}
		tempReader = tempReader->prevData;
		puts("");
		counter_1++;
		counter_2 = 0;
	}
}

void initialArray()
{
	assert(nCore > 0);
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


void freeArray()
{
	assert(nCore > 0);
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

void term(){
	puts("zakonczono");
	freeArray(4);
	done = 1;
}

void checkData() {
	FILE* data = NULL;
	char line[200];
	int counter = 0;
	
	
	assert(nMember == 0);
	assert(nCore == 0);
	
	data = fopen("/proc/stat", "r");
	assert(data!=NULL);
	
	fgets(line, 200, data);
	while (line[counter] != '\0')
	{
		if ((line[counter]) == ' ')
		{
			(nMember)++;
		}
		counter++;
	}
	(nMember)--;
	
	do{
		fgets(line, 200, data);
		(nCore)++;
	}while((strtok(line, " "))[0] == 'c'); // c is the first char of CPU
	(nCore)--;
	fclose(data);
}

void checkDataTest() {
	int a = 0;
	int b = 0;
	checkData(&a, &b);
	assert(a > 0 && b > 0);
	assert(a == 4);			//spec value for my system
	assert(b == 10); 		//spec value for my system
}

void makeDataOld()
{
	CPUstat* temp = ArrayOfData[0];
	ArrayOfData[0] = ArrayOfData[1];
	ArrayOfData[1] = temp;
}

void getData() {
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

void printCPUstat(int dtype) { // dtype - data type 1 - old 0 - new;
	int counter_1 = 0;
	int counter_2 = 0;
	
	assert(nCore > 0);
	
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

void setColor(double value, double levelA, double levelB, double levelC, double levelD){
	if (value >= levelA && value < levelB){
		printf("\033[0;32m");
	}
	else {
		if (value >= levelB && value < levelC){
			printf("\033[0;34m");
		}
		else {
			if (value >= levelC && value < levelD){
				printf("\033[0;33m");
			}
			else {
				if (value >= levelD){
				printf("\033[0;31m");
				}
				else {
					printf("\033[0m");
				}
			}
		}
	}
}

void printCPUusage(){
	int counter = 0;
	system("clear");
	while(counter <nCore){
		setColor(ArrayOfData[0][counter].usage, 1, 5, 10,20);
		printf("%s %5.2lf %%\n", ArrayOfData[0][counter].name, ArrayOfData[0][counter].usage);
		counter++;
	}
	printf("\033[0m");
}

/*
	0	|1	 |2		|3	 |4		|5	|6		|7	  |8	|9 
	user|nice|system|idle|iowait|irq|softirq|steal|guest|guestnice
	 
	0 - new data
	1 - old data
*/



void calcCPUusage(){
	unsigned long long int PrevIdle 	= 0;
	unsigned long long int Idle 		= 0;
	unsigned long long int PrevNonIdle 	= 0;
	unsigned long long int NonIdle 		= 0;
	unsigned long long int PrevTotal 	= 0;
	unsigned long long int Total 		= 0;
	unsigned long long int totald 		= 0;
	unsigned long long int idled 		= 0;
	
	for(int i = 0;i < nCore ;i++){
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

void *threadReader(){
	getData();
	calcCPUusage();
	return NULL;
}

void *threadAnalyzer(){
	calcCPUusage();
	return NULL;
}

void *threadPrinter(){
	printCPUusage();
	return NULL;
}


int main(int argc, char* argv[]) {
	
	
	int counter_main = 0;
	
	while(counter_main < 200){
		addDataQueue();
		printf("Iter: %d ILOSC DANYCH: %d \n", counter_main,dataReader->nIter);
		counter_main++;
	}	
	destroyQueue(dataReader);
	/*
	
	addDataQueue();
	printStat();
	sleep(1);
	addDataQueue();
	printStat();
	sleep(1);
	addDataQueue();
	printStat();
	
	rewindData(3);
	printf("ILOSC DANYCH po przewinieciu: %d \n", dataReader->nIter);
	printStat();
	rewindData(0);
	printf("ILOSC DANYCH: %d \n", dataReader->nIter);
	printStat();
	printf("ILOSC DANYCH: %d \n", dataReader->nIter);
	printf("poprzedni: %p | nastepnt: %p | iter: %d \n", dataReader->prevData->prevData, dataReader->prevData->nextData, dataReader->prevData->nIter);
	assert(dataReader != NULL);
	destroyQueue(dataReader->prevData->prevData->prevData->prevData);
	printf("poprzedni: %p | nastepnt: %p | iter: %d \n", dataReader->prevData, dataReader->nextData, dataReader->nIter);
	puts("Po usunieciu");
	printStat();
	//printf("poprzedni: %p | nastepnt: %p | iter: %d", dataReader->prevData, dataReader->nextData, dataReader->nIter);
	printf(" %p \n", dataReader);
	destroyQueue(dataReader);
	printf(" %p \n", dataReader);
	//destroyQueue(dataReader);
	//printf(" %p \n", dataReader);
	//int testIteration = 0;
	*/
	/*
	 * struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	checkData();
	initialArray();
	getData();
	pthread_t thread[5];
	pthread_create(&thread[2], NULL, &threadPrinter, NULL);
	while(!done){
		pthread_create(&thread[0], NULL, &threadReader, NULL);
		//pthread_create(&thread[1], NULL, &threadAnalyzer, NULL);
		if (pthread_join(thread[2], NULL) == 0) {
			pthread_create(&thread[2], NULL, &threadPrinter, NULL);
		}
		sleep(1)
		getData(nCore);
		calcCPUusage(nCore);ny
		system("clear");
		printCPUusage(nCore);
		//printCPUstat(nCore,1);
		//printCPUstat(nCore,0);
		//checkDataTest();
		testIteration++;
	}
	* 
	*/
	return 0;
}
