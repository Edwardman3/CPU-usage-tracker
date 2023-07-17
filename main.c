#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAX_SIZE_OF_QUEUE 20000


typedef struct ProcStat 
{
	char name[5];					// Name of CPU
	unsigned long long int* member;	// Array of data for one CPU 
	int nMember;					// Count of data for one CPU
	double usage;					// Procent usage of CPU
} CPUstat;

struct queue {
	struct queue* prevData;				// previous data in a queue if NULL its the last
	CPUstat actualData;				// data in a queue at position
	struct queue* nextData;				// next data in a queue	if NULL its the latest
	int nIter;						// id of the data
};


static CPUstat** ArrayOfData;
static struct queue* dataReader;
static pthread_t th[5];
static pthread_mutex_t lock;
static clock_t time_of_work[5];
static volatile sig_atomic_t done = 0;
static int nCore;
static int nMember;

static void destroyQueue(struct queue *fromData){
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
		else{
			dataReader = tempData;
		}
	}
}
		
static  void stopProgram(int sig){
	done = 0;
	printf("Signal: %d", sig);
	puts("\n CTR + C detected \n");
}

static void rewindData(int nIter){ // 0 set to the top, 1,2,3.. iter to down, -1, -2,-3,-4... iter to top
	int rewind_done = 1;
	int counter = 1;
	while(rewind_done){
		if(nIter > 0){
			if((dataReader->prevData != NULL) && (counter <= nIter)){
				dataReader = dataReader->prevData;
			}
			else{
				rewind_done = 0;
			}
		}
		else{
			if(nIter < 0){
				if((dataReader->nextData != NULL) && (counter <= (-1)*nIter)){
					dataReader = dataReader->nextData;
				}
				else{
					rewind_done = 0;
				}
			}
			else{
				if(dataReader->nextData != NULL){
					dataReader = dataReader->nextData;
				}
				else{
					rewind_done = 0;
				}
			}
		}
		counter++;
	}
}

static int addDataQueue(){
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
	//puts("START");
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
				rewindData(MAX_SIZE_OF_QUEUE - (nCore));
				//printf("%d \n", dataReader->nIter);
				destroyQueue(dataReader);
				rewindData(0);
			//	printf("%d \n", dataReader->nIter);
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
		dataReader->actualData.member = (unsigned long long int*)malloc((unsigned long)nMember * sizeof(unsigned long long int));
		
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

static void initialArray(){
	
	int counter_1 = 0;
	int counter_2 = 0;
	assert(nCore > 0);
	
	ArrayOfData = (CPUstat **)calloc(2, sizeof(CPUstat *));
	if (ArrayOfData != NULL){
		
		while(counter_1 < 2){
			*(ArrayOfData+counter_1) = (CPUstat*)calloc((unsigned long)nCore ,sizeof(CPUstat));
			
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
					(*(ArrayOfData+counter_1)+counter_2)->member = (unsigned long long int*)calloc((unsigned long)(*(ArrayOfData+counter_1)+counter_2)->nMember, sizeof(unsigned long long int));
					
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

static void freeArray(){
	
	int counter_1 = 0;
	int counter_2 = 0;
	assert(nCore > 0);
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

static int checkData() {
	FILE* data = NULL;
	char line[200];
	int counter = 0;
	
	errno = 0;
	data = fopen("/proc/stat", "r");
	if (errno != 0){
		perror("Error occured. File not open");
		return 1;
	}
	
	assert(nMember == 0);
	assert(nCore == 0);
	
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
	return 0;
}

/*
void checkDataTest() {
	int a = 0;
	int b = 0;
	checkData();
	assert(a > 0 && b > 0);
	assert(a == 6);			//spec value for my system
	assert(b == 10); 		//spec value for my system
}
*/

/*
void makeDataOld(void){
	CPUstat* temp = ArrayOfData[0];
	ArrayOfData[0] = ArrayOfData[1];
	ArrayOfData[1] = temp;
}
*/

static void setColor(double value, double levelA, double levelB, double levelC, double levelD){
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

static void printCPUusage(){
	int counter = 0;
	system("clear");
	while(counter <nCore){
		setColor(ArrayOfData[0][counter].usage, 1, 5, 10,20);
		printf("%s %5.2lf %%\n", ArrayOfData[0][counter].name, ArrayOfData[0][counter].usage);
		counter++;
	}
	printf("\033[0m");
}

static void getArrayFromQueue(){
	int temp_rewind_iter = 0;
	int counter_1 = 0;
	int	counter_2 = 0;
	
	rewindData(nCore);
	
	if(dataReader == NULL){	
		rewindData(1);
		
	}
	
	while((dataReader->nIter % nCore != 0) && (dataReader->actualData.member!=NULL)){
		rewindData(1);
		
	}
	while(counter_1 < nCore) {
		strcpy(ArrayOfData[0][counter_1].name, dataReader->actualData.name);
		while(counter_2 < ArrayOfData[0][counter_1].nMember) {
			ArrayOfData[0][counter_1].member[counter_2] = dataReader->actualData.member[counter_2];
			counter_2++;
		}
		rewindData(1);
		counter_1++;
		counter_2 = 0;
	}
	
	counter_1 = 0;
	counter_2 = 0;
	temp_rewind_iter = dataReader->nIter - nCore;
	rewindData(temp_rewind_iter);
	
	while(counter_1 < nCore) {
		
		strcpy(ArrayOfData[1][counter_1].name, dataReader->actualData.name);
		while(counter_2 < ArrayOfData[1][counter_1].nMember) {
			ArrayOfData[1][counter_1].member[counter_2] = dataReader->actualData.member[counter_2];
			counter_2++;
		}
		rewindData(1);
		counter_1++;
		counter_2 = 0;
	}
	rewindData((-1)*(temp_rewind_iter+(2*nCore)-1));

	destroyQueue(dataReader);

	rewindData(0);

}

static void calcCPUusage(){
	/*
	0	|1	 |2		|3	 |4		|5	|6		|7	  |8	|9 
	user|nice|system|idle|iowait|irq|softirq|steal|guest|guestnice
	 
	0 - new data
	1 - old data
	*/	
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
		pthread_mutex_lock(&lock);
		if(totald != 0){
			ArrayOfData[0][i].usage = 100*(double)(totald - idled)/(double)totald;
		}
		else{
			ArrayOfData[0][i].usage = 0;
		}
		pthread_mutex_unlock(&lock);
	}
}

static void *threadReader(){
	addDataQueue();
	return NULL;
}

static void *threadAnalyzer(){
	if (dataReader != NULL){
		if (dataReader->nIter >= 5000){
			pthread_mutex_lock(&lock);
			getArrayFromQueue();
			pthread_mutex_unlock(&lock);
			calcCPUusage();
		}
	}
	return NULL;
}

static void *threadPrinter(){
	pthread_mutex_lock(&lock);
	printCPUusage();
	pthread_mutex_unlock(&lock);
	return NULL;
}

//I dont know why but more then 3 thread wasnt working :/ 
/*
static void *threadWatchdog(){
	
	return NULL;
}
*/

/*
void testAnalyzer(void){
	
	int count = 40000;
	checkData();
	assert(dataReader != NULL);
	initialArray();
	assert(ArrayOfData != NULL);
	while(count>0){
		addDataQueue();
		count--;
	}
	//printf("Liczba danych: %d , nastepne dane: %p \n ",dataReader->nIter, dataReader->nextData);
	getArrayFromQueue();
	//printf("Liczba danych: %d , nastepne dane: %p \n ",dataReader->nIter, dataReader->nextData);
	calcCPUusage();
    //printf("Liczba danych: %d , nastepne dane: %p , poprzednie dane: %p \n ",dataReader->nIter, dataReader->nextData, dataReader->prevData);
	printCPUusage();
	//puts("test END");
	addDataQueue();
}
*/ 

int main(void) {
	int counter_task = 0;
	int ended_task [5] ={0,0,0,0,0};
	
	//pthread_t th[5];
	//clock_t time_of_work[5];
	
	done = 1;
	signal(SIGINT, stopProgram);
	checkData();
	assert(nCore > 0);
	assert(nMember > 0);
	initialArray();
	assert(ArrayOfData != NULL);
	
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("MUTES INIT FAILED!!!");
		return 1;
	}
	time_of_work[2] = time(NULL);
	while(done)
	{
		if (pthread_join(th[0], NULL) != 0 ){
			pthread_create(&(th[0]), NULL, &threadReader, NULL);
			time_of_work[0] = time(NULL);
		}
		
		if (pthread_join(th[1], NULL) != 0 ){
			pthread_create(&(th[1]), NULL, &threadAnalyzer, NULL);
			time_of_work[1] = time(NULL);
		}
		
		if ((pthread_join(th[2], NULL) != 0 ) && (time(NULL)-time_of_work[2] >= 1)){
			pthread_create(&(th[2]), NULL, &threadPrinter, NULL);
			time_of_work[2] = time(NULL);
		}
/*
		if (pthread_join(th[3], NULL) != 0 ){
			pthread_create(&(th[3]), NULL, &threadWatchdog, NULL);
			time_of_work[3] = time(NULL);
		}
	*/	
	}
	

	while(counter_task < 3){
		while(ended_task[counter_task] == 0){
			ended_task[counter_task] = pthread_join(th[counter_task], NULL);
			if (ended_task[counter_task]){
				printf("End task %d \n",counter_task + 1);
			}
		}
		counter_task++;
	}
	
	
	pthread_mutex_destroy(&lock);
	destroyQueue(dataReader);
	freeArray();
	
	puts("Closing the program!");
	
	return 0;
}
