#include <linux/unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

//Struct for the nodes in the linked list
struct Node {
	struct task_struct *process;
	struct Node *next;
};
//Struct for the semaphore
struct cs1550_sem {
	int value;
	struct Node *first;
	struct Node *last;
};


//Define wrappers for down and up
void down(struct cs1550_sem *sem) {
	syscall(__NR_cs1550_down, sem);
}

void up(struct cs1550_sem *sem) {
	syscall(__NR_cs1550_up, sem);
}

int main(int argc, char *argv[]) {
	//Get command line arguments
	int numOfCons = atoi(argv[1]);	//# of consumers
	int numOfProds = atoi(argv[2]);	//# of producers
	int bufferSize = atoi(argv[3]);	//Size of the buffer
	

	//Create a buffer the size of 3 semaphore structs
	void *ptr = mmap(NULL, 3 * sizeof(struct cs1550_sem), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	//Initialze the three semaphore structs
	struct cs1550_sem *mutex;
	struct cs1550_sem *empty;
	struct cs1550_sem *full;

	//Place the three semaphores into the buffer
	mutex = (struct cs1550_sem*) ptr;
	empty = (struct cs1550_sem*) ptr + 1;
	full = (struct cs1550_sem*) ptr + 2;

	//Initialize the values of the three semaphores
	empty->value = bufferSize;
	full->value = 0;
	mutex->value = 1;

	empty->first = NULL;
	empty->last = NULL;
	full->first = NULL;
	full->last = NULL;
	mutex->first = NULL;
	mutex->last = NULL;



	//Create a buffer for the producer and consumer to store their items
	//The size of the buffer is specified 
	void *prodConsMemory = mmap(NULL, sizeof(int) * (bufferSize + 3), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	int *producerPtr;
	int *consumerPtr;
	int *buffer;
	int *n;
	//Set the producer pointer to the beginning of the buffer
	producerPtr = (int *)prodConsMemory;
	//Store the consumer pointer in the same buffer at an offset of 1
	consumerPtr = (int *)prodConsMemory + 1;
	//Store the size of the buffer at an offset of 2
	n = (int *)prodConsMemory + 2;
	//Set the buffer to begin at an offset of 3
	buffer = (int *)prodConsMemory + 3;


	//Set the initial values for the producer and consumer pointers to 0
	*producerPtr = 0;
	*consumerPtr = 0;
	//Set n equal to the size of the buffer
	*n = bufferSize;


	//Create for loop counter
	int i;

	for (i = 0; i < bufferSize; i++)
		buffer[i] = 0;

	//Loop for the number of producers, from the slides
	for (i = 0; i < numOfProds; i++) {
		if (fork() == 0) {
			int prodItem;
			while (1) {
				//Down on the empty and mutex semaphores
				down(empty);
				down(mutex);

				//Place the item in the buffer
				prodItem = *producerPtr;
				buffer[*producerPtr] = prodItem;
				printf("Producer %c produced: %d\n", (i + 65), prodItem);
				*producerPtr = (*producerPtr + 1) % *n;

				//Up on the mutex and full semaphore
				up(mutex);
				up(full);
			}
		}
	}

	//Loop for the nuber of consumers, from the slides
	for (i = 0; i < numOfCons; i++) {
		if (fork() == 0) {
			int consItem;
			while (1) {
				down(full);
				down(mutex);

				consItem = buffer[*consumerPtr];
				printf("Consumer %c consumed: %d\n", (i + 65), consItem);
				*consumerPtr = (*consumerPtr + 1) % *n;

				up(mutex);
				up(empty);
			}
		}

	}

	int status;
	wait(&status);
	return 0;
	
}

