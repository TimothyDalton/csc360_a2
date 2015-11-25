#include <stdio.h>
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>     /* General Utilities */

#include <string.h>
#include <pthread.h>
#include <time.h>

// typedef int bool;

// #define true 1
// #define false 0

// bool shopOpen = false;	//do not allow any customers to be serviced until
// 						//'shop opens' aka all customers from the textfile
// 						//are initialized into their appropriate threads

//Current Customers Thread

#define currentCustomerNumber customers[i][0]
#define currentCustomerArrival customers[i][1]
#define currentCustomerServiceTime customers[i][2]
#define currentCustomerPriority customers[i][3]
#define currentCustomerStatus customers[i][4]

//Possible Next-in-Line Customers Thread (for determining ties in Simulation Rules )

#define nextCustomerNumber customers[j][0]
#define nextCustomerArrival customers[j][1]
#define nextCustomerServiceTime customers[j][2]
#define nextCustomerPriority customers[j][3]
#define nextCustomerStatus customers[j][4]

pthread_cond_t convar1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int customerWaiting = -1;

int** customers;				//pointer to the customers


int promptInputChecker(int argc,char* argv[],FILE *textFile)
{
	if(argc < 2)
	{
		printf("\nToo few parameters given\n");
		return 1;
	}

	else if(argc > 2)
	{
		printf("\nToo many parameters given\n");
		return 1;
	}

	if(textFile == NULL)
	{
		printf("\nTextfile used is NULL,unreadable, or does not exist\n");
		return 1;
	}

	int fclose(FILE *textFile);			//else valid!
	return 0;

}

// Starting place for flows (okay, threads representing flows)
void *customerServicer(void* customerIndex)
{

	long i = (long)customerIndex;				//current customers index
		
	// Wait for variable to change from -1 to 0 (by main) and go!
	while(customerWaiting < 0){}				//customers can go when 'openShop'
												//changed from using a bool
												//because need more than 2 options for later
	
	
	double startClock = (double)clock()/CLOCKS_PER_SEC;
	
	// Use usleep to stall until arrival time
	int clockTimer = 0;
	
	while(clockTimer++ < currentCustomerArrival)		//run clock time until a customer arrives
	{
		usleep(100000);
	}
	

	currentCustomerStatus = 1;							//customer arrives
	
	double stopClock = (double)clock()/CLOCKS_PER_SEC;

	printf("customer %d arrives: arrival time %.2f, service time %.1f, priority %d.\n",
		currentCustomerNumber,stopClock-startClock,(double)currentCustomerServiceTime/10,currentCustomerPriority);

	//wait until status changed from arrive to transmit. if customer is only arrived
	//it does not have token at this point

	if((currentCustomerStatus == 1)&&(customerWaiting > 0)&&(currentCustomerStatus != customerWaiting))
	{
		printf("customer %d waits for the finish of customer %d.\n",currentCustomerStatus,customerWaiting);
	}

	
	pthread_mutex_lock(&mutex1);
	
	while(currentCustomerStatus == 1)				//lock so can work on thread
	{
		pthread_cond_wait(&convar1,&mutex1);
	}

	
	
	customerWaiting = currentCustomerNumber;		//this is now customer others are waiting on
		

	startClock = (double)clock()/CLOCKS_PER_SEC - startClock;
	
	printf("The clerk starts serving customer %d at time %.2f.\n",currentCustomerNumber,startClock);
	
	clockTimer = 0;
	while(clockTimer++ < currentCustomerServiceTime)		//calculate service time in seconds
	{
		usleep(100000);
	}
	
	stopClock = (double)clock()/CLOCKS_PER_SEC;
	
	printf("The clerk finishes the service to customer %d at time %.2f\n",currentCustomerNumber,stopClock);
	
	currentCustomerStatus = 3;		//customer finished
	
	pthread_mutex_unlock(&mutex1);
	
	pthread_exit(0);
}


int customerTotalServed(int totalCustomers)		//count total # of customers served so far
{
	int customersServed = 0;
	int i = 0;
	for(i=0;i<totalCustomers;i++)
	{
		if(currentCustomerStatus == 3)
		{
			customersServed++;
		}
	}
	return customersServed;
}

// Sends the next flow, if one is available
int findNextCustomerToService(int totalCustomers)
{
	
	int i = 0;		//current customer
	int j = 0;		//possible next customer
	

	while((currentCustomerStatus != 1)&&(++i < totalCustomers-1)){}		//find first ready customer
	
	if(currentCustomerStatus != 1)		//no customers ready
	{
		return 1;
	}
	

	for(j=i;i<totalCustomers;i++)
	{
		if(currentCustomerStatus == 1)		//customer arrived
		{
			if(nextCustomerPriority < currentCustomerPriority)		//current customer higher priority
			{
				j = i;
			}
			else if(currentCustomerPriority == nextCustomerPriority)
			{

				if(nextCustomerArrival < currentCustomerArrival)		//equal priority, compare
				{
					j = i;
				}
				else if(currentCustomerArrival == nextCustomerArrival)
				{
					// If equal then compare transmission times
					// ...and tie goes to first in the list (that's j)
					if(nextCustomerServiceTime < currentCustomerServiceTime)
					{
						j = i;
					}
				}
			}
		}
	}
	
	nextCustomerStatus = 2;		//next customer to be served
	
	pthread_cond_broadcast(&convar1);		//signal flag change for next customer up
	
	while(nextCustomerStatus == 2){}		//customer 2 service

	return 0;
}


int main(int argc, char* argv[])
{
	
	char stringCustomers[50];					//string array placeholder for
												//# of customers

	
	FILE *textFile = fopen(argv[1],"r");		//pass: 
												//	string array of # of customers
												//	total user prompt arguments (argc)
	if(promptInputChecker(argc,argv,textFile))	//	user prompt at [i] (argv)
	{											//to check for legal format PQS customers.txt
		int fclose(FILE *textFile);
		return 1;
	}



	fgets(stringCustomers,50,textFile);			//get total # of customers (as a char)
												//is first spot in proper textFile format
	

	int totalCustomers = atoi(stringCustomers);	//convert total to int
												//and store as totalCustomers

	customers = malloc( sizeof(int*)*totalCustomers);

	int i=0;

	for(i=0;i<totalCustomers;i++)
	{
		customers[i] = malloc( sizeof(int)*5);
	}

	pthread_t customerThreads[totalCustomers];

	i = 0;

	while(i<totalCustomers)
	{
		fgets(stringCustomers,50,textFile);

		currentCustomerNumber = atoi(strtok(stringCustomers,":"));		//customer number
		currentCustomerArrival = atoi(strtok(NULL,"," ));		//customer arrival time
		currentCustomerServiceTime = atoi(strtok(NULL,"," ));		//customer service time
		currentCustomerPriority = atoi(strtok(NULL,"\n"));		//customer priority
		currentCustomerStatus = 0;							//customer status where
															//[i][4] = 0: created
															//[i][4] = 1: arrived
															//[i][4] = 2: being serviced
															//[i][4] = 3: finished service

		if((currentCustomerNumber < 0) || (currentCustomerArrival < 0) || (currentCustomerServiceTime < 0) || 
				(currentCustomerPriority < 0) || (currentCustomerStatus < 0))
		{
			printf("Negative values in textFile");
			return 1;
		}

		pthread_create(&customerThreads[i],NULL,customerServicer,(void *)(long)i);	//create customer thread at index

		i++;


	}


	int fclose(FILE *inputFile);			//done reading in textFile and creating threads

	customerWaiting = 0;					//'shopOpen'

	while(customerTotalServed(totalCustomers)<totalCustomers)	//keeps checking for customers
	{															//until all served
		findNextCustomerToService(totalCustomers);
	}
	
	for(i=0;i<totalCustomers;i++)
	{
		free(customers[i]);
	}

	free(customers);

	printf("\nProgram Ended Successfully\n");

	return 0;

}