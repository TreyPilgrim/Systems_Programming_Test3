// Main
/*
  1. Displays "ascii modern art" composed of a series of
     randomly generated blocks of 2 or more repeating characters

  1. Create a semaphore set of size 2 -- using IPC_PRIVATE as your key
     a. Initialize the semaphore representing the child to available
     b. Initialize the semaphore representing the parent to in use
  2. Create a segment of shared memory -- use IPC_PRIVATE as your key
  3. Create a child process that:
     a. Attaches the segment of shared memory
     b. Seeds the random number generator
     c. Reserves the child semaphore
     d. Generates and stores the total number of blocks to generate.
        - This should be between 10 and 20
     e. For each block, generate and store following values:
        1. The length of the block (i.e. the number of characters to display)
	   - Between 2 and 10
	2. The character that comprises the block
	   - Between 'a' and 'z'
     f. Release the parent semaphore 
     g. Reserve the child semaphore
     h. Detach the shared memory segment
     i. Release the parent semaphore
  4. Create a parent process that:
     a. Attaches the segment of shared memory
     b. Seeds the random number generator
     c. Reserves the parent semaphore
     d. Generates a random width for the ASCII art
        - Between 10 and 15
     e. Using the data stored in the shared memory segment, output an image
        - Tips:
	 1. One value stored in the segment should represnt the number
	    of (Length, character) pairings. For each (Lenght, character)
	    pairing, you should output length instances of the given
	    character. This means if the pairing was (3, 'b'), you
	    would output "bbb".
	 2. The random image has basically been encoded use run-length
	    encoding (RLE); RLE doesn't include the location of new lines.
	    The location of new lines is determined by the random width
	    generated in step d. Each time you output width total characters,
	    output a new line.
     f. Release the child semaphore
     g. Reserve the parent semaphore
     h. Detach the shared memory segment
  5. Delete the semaphore
  6. Delete the shared memory

  -- You can/should use the binary semaphore protocol indroduced in class
  
  
 */

/* Problem 3 -- List the include files for this program */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>

#include "semun.h"
#include "binary_sem.h"

#define PRODUCER 0
#define CONSUMER 1
// The colors
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

int checkError(int val, const char *msg)
{
  if (val == -1)
    {
      perror(msg);
      exit(EXIT_FAILURE);
    }

  return val;
}

/* Problem 4 -- Declare a structure that represents the data stored
                in the shared memory */
struct shmMem
{
  int number; // # of blocks to generate
  int length[1023]; // Length of the block (i.e. 5 a's)
  char val[1023]; // The char to be displayed
  int color [1023]; // color @ i
};
//-------------------------------------------------------------------------
// Get Random #
int genRand(int low, int high)
{
  int rng = high - low + 1;
  double drnd = rand();
  int irnd = drnd/ ((double) RAND_MAX + 1) * rng;
  return low + irnd;
}
//-------------------------------------------------------------------------
// Get Random Char
char genRandChar(char low, char high)
{
    int rng = high - low + 1;
    int irnd = rand() % rng; // Use % to get a value in the range [0, rng-1]
    return low + irnd;
}
//-------------------------------------------------------------------------
// Print top and bottom border Function
void printLine(int width)
{
  int i;
  printf("+");
  for(i = 0; i < width; i++)
    printf("-");

  printf("+\n");
}
//---------------------------------------------------------------------------
/* Problem 5 -- Create a function to handle the code for the child.
                Be certain to pass this function the id values for
		both the semaphore and the shared memory segment */
void producer(int semid, int shmid)
{
  
  struct shmMem *addr;
  int cnt = 0;
  int val = 0;
  int colour = 0;
  char letter = ' ';
  int i;

  /* 3a. Attaches the segment of shared memory */
  addr = (struct shmMem *) shmat(shmid, NULL, 0);
  /* 3b. Seeds the random number generator */
  srand(time(NULL));


  /* 3c. Reserves the child semaphore */
  reserveSem(semid, PRODUCER);

  /* 3d. Generates and stores the total number of blocks to generate. -- Between 10 and 20 */
  cnt = rand() % 20 + 10;
  addr->number = cnt;

  for (i = 0; i < cnt; i++)
    {
      /* 3e. For each block, generate and store following values */
      /* 3e1. The length of the block -- Between 2 and 10 */
      val = genRand(2, 10);
      addr->length[i] = val;

      /* 3e2. The character that comprises the block -- between 'a' and 'z' */
      letter = genRandChar('a', 'z');
      while(letter == addr->val[i-1]) // No repeating char vals
	letter = genRandChar('a', 'z');
      addr->val[i] = letter;

      // Determine the color for the segment -- between 1 and 6
      colour = genRand(1, 6);
      while (colour == addr->color[i-1]) // No repeating colors
	colour = genRand(1, 6);
      
      addr->color[i] = colour;
      
    }

  /* 3f. Release the parent semaphore */
  releaseSem(semid, CONSUMER);

  /* 3g. Reserve the child semaphore */
  reserveSem(semid, PRODUCER);
  
  /* 3h. Detach the shared memory segment */
  shmdt(addr);

  /* 3i. Release the parent semaphore */
  releaseSem(semid, CONSUMER);

  /* 6. Delete the shared memory  */
  checkError(shmctl(shmid, 0, IPC_RMID), "shmctl");
  /* 5. Delete the semaphore  */
  checkError(semctl(semid, 0, IPC_RMID), "semctl");
}
//-------------------------------------------------------------------
/* Problem 6 -- Create a function to handle the code for the parent.
                Be certain to pass this function the id values for both the
		semaphore and the shared memory segment */
void consumer(int semid, int shmid)
{
  int width = 0; // random width for lines -- Btw 10 and 15
  int blocks = 0; // counter for blocks
  int length = 0; // # of chars printed
  int colour = 0; // color chosen
  char val = ' '; // value to be printed
  int cnt = 0; // counter for width 
  
  

  /* 4a. Attaches the segment of shared memory */
  struct shmMem *addr;
  addr = (struct shmMem *) shmat(shmid, NULL, 0);

  /* 4b. Seeds the random number generator */
  srand(time(NULL));

  /* 4c. Reserves the parent semaphore */
  reserveSem(semid, CONSUMER);

  /* 4d. Generates a random width for the ASCII art -- Between 10 and 15 */
  width = genRand(10, 15); 

  // Top Border
  printLine(width);

  while(blocks < addr->number)
    {
      val = addr->val[blocks];
      length = addr->length[blocks];
      colour = addr->color[blocks];
      int i;

      /* 4e. Print the block with ASCII colors */
      switch(colour)
      {
      case 1:
	printf(RED); // print in red

	for(i = 0; i < length; i++)
	  {
	    // Print outer borders
	    if (cnt == 0 || cnt == width)
	      {
		printf(RESET);
		printf("|");

		if (cnt == width)
		  {
		    printf("\n|");  // Print newline and outer border if at width
		    cnt = 0; // Reset count if starting a new line
		  }

		printf(RED);
	      }
	    
	    printf("%c", val);
	    ++cnt; // keep up with the count
	  }
	
	// reset color
	printf(RESET);
	++blocks; // increment the block counter
	break;

      case 2:
	printf(GREEN); // print in green

	for(i = 0; i < length; i++)
	  {
	    // Print outer borders
	    if (cnt == 0 || cnt == width)
	      {
		printf(RESET);
		printf("|");

		if (cnt == width)
		  {
		    printf("\n|");  // Print newline and outer border if at width
		    cnt = 0; // Reset count if starting a new line
		  }
		printf(GREEN);
	      }

	    printf("%c", val);
	    ++cnt; // keep up with the count	    
	  }
	
	// reset color
	printf(RESET);
	++blocks; // increment the block counter
	break;

      case 3:
	printf(YELLOW); // print in yellow


	for(i = 0; i < length; i++)
	  {
	    // Print outer borders
	    if (cnt == 0 || cnt == width)
	      {
		printf(RESET);
		printf("|");

		if (cnt == width)
		  {
		    printf("\n|");  // Print newline and outer border if at width
		    cnt = 0; // Reset count if starting a new line
		  }

		printf(YELLOW);
	      }

	    printf("%c", val);
	    ++cnt; // keep up with the count
	  }
	
	// reset color
	printf(RESET);
	++blocks; // increment the block counter
	break;

      case 4:
	printf(BLUE); // print in blue


	for(i = 0; i < length; i++)
	  {
	    // Print outer borders
	    if (cnt == 0 || cnt == width)
	      {
		printf(RESET);
		printf("|");

		if (cnt == width)
		  {
		    printf("\n|");  // Print newline and outer border if at width
		    cnt = 0; // Reset count if starting a new line
		  }
		printf(BLUE);
	      }
	    
	    printf("%c", val);
	    ++cnt; // keep up with the count
	  }
	
	// reset color
	printf(RESET);
	++blocks; // increment the block counter
	break;

      case 5:
	printf(MAGENTA); // print in magenta


	for(i = 0; i < length; i++)
	  {
	    // Print outer borders
	    if (cnt == 0 || cnt == width)
	      {
		printf(RESET);
		printf("|");

		if (cnt == width)
		  {
		    printf("\n|");  // Print newline and outer border if at width
		    cnt = 0; // Reset count if starting a new line
		  }
		printf(MAGENTA);
	      }
	    
	    printf("%c", val);
	    ++cnt; // keep up with the count
	  }
	
	// reset color
	printf(RESET);
	++blocks; // increment the block counter
	break;

      case 6:
	printf(CYAN); // print in cyan


	for(i = 0; i < length; i++)
	  {
	    // Print outer borders
	    if (cnt == 0 || cnt == width)
	      {
		printf(RESET);
		printf("|");

		if (cnt == width)
		  {
		    printf("\n|");  // Print newline and outer border if at width
		    cnt = 0; // Reset count if starting a new line
		  }

		printf(CYAN);
	      }
	    
	    printf("%c", val);
	    ++cnt; // keep up with the count
	  }
	
	// reset color
	printf(RESET);
	++blocks; // increment the block counter
	break;

      }	
         
    }
  // Print last border
  while(cnt != width)
    {
      printf(" ");
      ++cnt;
    }
  // Print bottom of chart
  printf("|\n");
  printLine(width);

  /* 4f. Release the child semaphore */
  releaseSem(semid, PRODUCER);

  /* 4g. Reserve the parent semaphore */
  reserveSem(semid, CONSUMER);
  
  /* 4h. Detach the shared memory segment */
  shmdt(addr);
  
}
//------------------------------------------------------------------
/* Problem 7 -- Implement the function main */


int main(int argc, char *argv[])
{
  int semid, shmid;
  pid_t child;
  
  /* 1. Create a semaphore set of size 2 -- using IPC_PRIVATE as your key */
  semid = checkError(semget(IPC_PRIVATE, 2, IPC_CREAT | S_IRUSR | S_IWUSR), "semget");
  /*  a. Initialize the semaphore representing the child to available */
  initSemAvailable(semid, PRODUCER);
  /*  b. Initialize the semaphore representing the parent to in use */
  initSemInUse(semid, CONSUMER);

  /* 2. Create a segment of shared memory -- use IPC_PRIVATE as your key */
  shmid = checkError(shmget(IPC_PRIVATE, sizeof(struct shmMem),IPC_CREAT | S_IRUSR | S_IWUSR), "shmget");

  /* 3. Create a child process */
  child = checkError(fork(), "fork");

  if (child == 0)
    {
      producer(semid, shmid);
      exit(EXIT_SUCCESS);
    }
  else
    {
      consumer(semid, shmid);
      exit(EXIT_SUCCESS);
    }

  return 0;
  
}
