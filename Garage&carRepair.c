#include <semaphore.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <pthread.h>
#include <unistd.h>
#define ever ;;
#define line 256

typedef struct resource
{
	int type;
	char *name;
	int numberOfSystems;
}resource;

typedef struct repair
{
	int type;
	char *name;
	int requiredTime;
	int numberOfResourcesNeeded;
	int *resourcesList;

}repair;

typedef struct request
{
	int licenseNumber;
	int arrivalTime;
	int numberOfRepairTypes;
	int *repairsList;

}request;

//global parameters//
int hour = 0, day = 1, howManyRequests, howManyRepairs, howManyResources;
resource *resourceArr, *tempResource;
request *requestArr, *tempRequest;
repair *repairArr, *tempRepair;
sem_t* sema; //semaphore array

void *timerFunc();
int countLines(FILE *filename);
int getRepairIdx(int id);
int getResourceIdx(int id);
void toRepairOperation(int licenseNumber, repair repair);
void * requestHandler(void* privateRequest);
void * repairHandler(void* privateRepair);

int main(int argc, char *argv[]) {
	//temps will be serve as the the first elemnt for the input recivement 
	FILE *resources, *repairs, *requests;
	char *name, *temp, str[line]; //temp and name, variables for reciving name strings
	int i, j, f, howManyLines; //i,j,f indexes
	pthread_t* requestsThread;
	pthread_t timerThread;

	//allocating the arrays
	resourceArr = (resource*)malloc(sizeof(resource));
	requestArr = (request*)malloc(sizeof(request));
	repairArr = (repair*)malloc(sizeof(repair));

	//allocating temp structs 
	tempResource = (resource*)malloc(sizeof(resource));
	tempRequest = (request*)malloc(sizeof(request));
	tempRepair = (repair*)malloc(sizeof(repair));

	//Dealing with the repairs
	i = 0; j = 0;
	repairs = fopen("repairs.txt", "r");
	if (repairs == NULL) {
		printf("failed to open file\n");
		exit(1);
	}
	howManyLines = countLines(repairs);
	howManyRepairs = howManyLines;
	for (f = 0; f < (howManyLines - 1); f++) {
		fgets(str, line, repairs);
		if (str[0] == '\n') {
			howManyRepairs -= 2; //substrcting the last line and the index 
			break;
		}
		tempRepair->type = atoi(strtok(str, "\t"));
		name = strtok(NULL, "\t");
		tempRepair->requiredTime = atoi(strtok(NULL, "\t"));
		tempRepair->numberOfResourcesNeeded = atoi(strtok(NULL, "\t"));

		repairArr[i].type = tempRepair->type;
		repairArr[i].name = (char*)malloc(sizeof(char)*(strlen(name) + 1));
		strcpy(repairArr[i].name, name); 
		repairArr[i].requiredTime = tempRepair->requiredTime;
		repairArr[i].numberOfResourcesNeeded = tempRepair->numberOfResourcesNeeded;

		repairArr[i].resourcesList = (int*)malloc(sizeof(int)); //creating the resource list
		temp = strtok(NULL, "\t");
		while (temp && atoi(temp) != 0) {
			repairArr[i].resourcesList[j] = atoi(temp);
			j++;
			repairArr[i].resourcesList = (int*)realloc(repairArr[i].resourcesList, (sizeof(int)*(j + 1)));
			temp = (strtok(NULL, "\t"));
		}

		i++;
		j = 0;
		repairArr = (repair*)realloc(repairArr, (sizeof(repair)*(i + 1)));

	}
	fclose(repairs);
	printf("#### Repairs were feeded ####\n\n");
	printf("------------------------------------\n");


	//Dealing with the resources
	i = 0; j = 0;
	resources = fopen("resources.txt", "r");
	if (resources == NULL) {
		printf("failed to open file\n");
		exit(1);
	}
	howManyLines = countLines(resources);
	howManyResources = howManyLines;
	for (f = 0; f < (howManyLines - 1); f++) {
		fgets(str, line, resources);
		if (str[0] == '\n') {
			howManyResources -= 2; //substrcting the last line and the index 
			break;
		}

		tempResource->type = atoi(strtok(str, "\t"));
		name = strtok(NULL, "\t");
		tempResource->numberOfSystems = atoi(strtok(NULL, "\t"));

		resourceArr[i].type = tempResource->type;
		resourceArr[i].name = (char*)malloc(sizeof(char)*(strlen(name) + 1));
		strcpy(resourceArr[i].name, name);
		resourceArr[i].numberOfSystems = tempResource->numberOfSystems;
		
		i++;
		resourceArr = (resource*)realloc(resourceArr, (sizeof(resource)*(i + 1)));

	}
	resourceArr = (resource*)realloc(resourceArr, (sizeof(resource)*i)); //allcating the correct size for the allocation 
	fclose(resources);
	printf("#### Resources were feeded ####\n\n");
	printf("------------------------------------\n");

	//Dealing with the requests
	i = 0; j = 0;
	requests = fopen("requests.txt", "r");
	if (requests == NULL) {
		printf("failed to open file\n");
		exit(1);
	}
	howManyLines = countLines(requests);
	howManyRequests = howManyLines;
	for (f = 0; f < (howManyLines - 1); f++) {
		fgets(str, line, requests);
		if (str[0] == '\n') {
			howManyRequests -= 2; //substrcting the last line and the index 
			break;
		}

		//allocating for the temp struct
		tempRequest->licenseNumber = atoi(strtok(str, " \t"));
		tempRequest->arrivalTime = atoi(strtok(NULL, " \t"));
		tempRequest->numberOfRepairTypes = atoi(strtok(NULL, " \t"));

		//feeding the arr's struct
		requestArr[i].licenseNumber = tempRequest->licenseNumber;
		requestArr[i].arrivalTime = tempRequest->arrivalTime;
		requestArr[i].numberOfRepairTypes = tempRequest->numberOfRepairTypes;
		requestArr[i].repairsList = (int*)malloc(sizeof(int) * requestArr[i].numberOfRepairTypes);
		for (j = 0; j < tempRequest->numberOfRepairTypes; j++)
			requestArr[i].repairsList[j] = atoi(strtok(NULL, " \t"));

		i++;
		requestArr = (request*)realloc(requestArr, (sizeof(request)*(i + 1)));
	}

	requestArr = (request*)realloc(requestArr, (sizeof(request)*i)); //reallocating again
	fclose(requests);
	printf("#### Requests were feeded ####\n\n");
	printf("------------------------------------\n");

	/////////////////////////////////////////////////////////////////////////////////////// START THE SIMULATION//////////////////////////////////////////////////////////////////////////
	requestsThread = (pthread_t*)malloc(sizeof(pthread_t)*howManyRequests);
	sema = (sem_t*)malloc(sizeof(sem_t)*howManyResources);

	for (i = 0; i < howManyResources; i++) // set all semaphores
		sem_init(&sema[i], 0, resourceArr[i].numberOfSystems);
	pthread_create(&timerThread, NULL, timerFunc, NULL); //start the timer 0-23

	for (i = 0; i < howManyRequests; i++)
		pthread_create(&requestsThread[i], NULL, requestHandler, (void*)&requestArr[i]);


	for (i = 0; i < howManyRequests; i++)   // wait for all requests to complete job
		pthread_join(requestsThread[i], NULL);

	////////////////////////////////////////////////////////////////////////////////////// FREE SECTION /////////////////////////////////////////////////////////////////////////////////////
	printf("starting resource freeing\n");
	for (i = 0; i < howManyResources; i++)
		free(resourceArr[i].name);
	free(resourceArr);
	free(tempResource);
	printf("free resource completed\n");

	printf("starting repair freeing\n");
	for (i = 0; i < howManyRepairs; i++) {
		free(repairArr[i].name);
		free(repairArr[i].resourcesList);
	}

	free(repairArr);
	free(tempRepair);
	printf("free repair completed\n");

	printf("start request freeing\n");
	for (i = 0; i < howManyRequests; i++)
		free(requestArr[i].repairsList);
	free(requestArr);
	free(tempRequest);
	printf("free request completed\n");

	printf("start requestsThread freeing\n");
	free(requestsThread);
	printf("free requestsThread completed\n");
	printf("start sema freeing\n");
	free(sema);
	printf("free sema completed\n");


	printf("## All the requests are done !!! ##\n");
	pthread_join(timerThread, NULL);
	return 0;
}

void *timerFunc() {
	for (ever) {
		if (hour == 24) {
			hour = 0;
			
		}

		else {
			
			hour++;
		}
		sleep(1);
	}
}

int countLines(FILE *filename) {// count the number of lines in the file called filename                                    
	int ch = 0;
	int lines = 0;

	lines++;
	while ((ch = fgetc(filename)) != EOF)
	{
		if (ch == '\n')
			lines++;
	}
	fseek(filename, 0, SEEK_SET); //back to the begning of the file 
	return lines;
}

int getRepairIdx(int id) { //searching for the correct index
	int j;
	for (j = 0; j < howManyRepairs; j++) {
		if (repairArr[j].type == id)
			return j;
	}
	return -1;
}

int getResourceIdx(int id) { //searching for the correct index
	int j;
	for (j = 0; j < howManyResources; j++) {
		if (resourceArr[j].type == id)
			return j;
	}
	return -1;
}

//checking all the requests
void *requestHandler(void* privateRequest) {
	request rst = *(request*)privateRequest;
	int i, j;
	int repairsIndex, resourceIndex;
	pthread_t* repairsThread;
	repairsThread = (pthread_t*)malloc(sizeof(pthread_t)*rst.numberOfRepairTypes);
	for (ever)
	{
		if (hour == rst.arrivalTime) //send request for disscussion    
		{
			printf("car: %d time: %d arrived to the garage.\n", rst.licenseNumber, hour);
			for (i = 0; i < rst.numberOfRepairTypes; i++) {
				repairsIndex = getRepairIdx(rst.repairsList[i]);
				pthread_create(&repairsThread[i], NULL, repairHandler, (void*)&rst.repairsList[i]); //crearing threads for each reapir as the number of the 
				printf("car: %d time: %d started %s.\n", rst.licenseNumber, hour, repairArr[repairsIndex].name);
				sleep(repairArr[repairsIndex].requiredTime);

				//realesing the resources for the repair
				for (j = 0; j < repairArr[repairsIndex].numberOfResourcesNeeded; j++) {
					resourceIndex = getResourceIdx(repairArr[repairsIndex].resourcesList[j]);
					sem_post(&sema[resourceIndex]);
				}

				printf("car: %d time: %d completed %s.\n", rst.licenseNumber, hour, repairArr[repairsIndex].name);
				pthread_join(repairsThread[i], NULL);
			}
			printf("car: %d time: %d service complete.\n", rst.licenseNumber, hour);
			free(repairsThread);
			return NULL;
		}
	}
}

//checking all the resources for each repair
void *repairHandler(void* privateRepair) {
	int i, repairsIndex, resourceIndex;
	int rpt = *(int*)privateRepair;
	repairsIndex = getRepairIdx(rpt);
	for (i = 0; i < repairArr[repairsIndex].numberOfResourcesNeeded; i++) {
		resourceIndex = getResourceIdx(repairArr[repairsIndex].resourcesList[i]);
		sem_wait(&sema[resourceIndex]); 
	}
	return NULL;
}




