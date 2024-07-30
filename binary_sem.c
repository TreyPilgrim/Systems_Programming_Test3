// Binary_sem.c
#include <sys/types.h>
#include <sys/sem.h>
#include "semun.h"
#include "binary_sem.h"

// Global Variables for configuring semaphore behavior
int bsUseSemUndo = 0; // Flag indicating whether to use SEM_UNDO for semaphores
int bsRetryOnEintr = 1; // Flag indicating whether to retry semaphore operations on EINTR

// Function to initialize a semaphore to an available state
int initSemAvailable(int semId, int semNum)
{
  union semun arg;
  arg.val = 1; // Set semaphore value to 1, indicating it is available
  return semctl(semId, semNum, SETVAL, arg);
}

// Function to initialize a semaphore to an in-use state
int initSemInUse(int semId, int semNum)
{
  union semun arg;
  arg.val = 0; // Set semaphore value to 0, indicating it is in use
  return semctl(semId, semNum, SETVAL, arg);
}

// Function to reserve a semaphore
int reserveSem(int semId, int semNum)
{
  struct sembuf sops;

  sops.sem_num = semNum;// Semaphore number within the semaphore set
  sops.sem_op = -1; // Operation to decrement semaphore value
  sops.sem_flg = bsUseSemUndo ? SEM_UNDO : 0; // Use SEM_UNDO if bsUseSemUndo is set

  // Loop to handle EINTR (interrupted system call)
  // and retry if bsRetryOnEintr is set
  while (semop(semId, &sops, 1) == -1)
    {
      if (errno != EINTR || !bsRetryOnEintr)
	return -1; // Return -1 if not interrupted or not retrying on EINTR
    }

  return 0; // Return 0 on successful semaphore reservation
}

// Function to release a semaphore
int releaseSem(int semId, int semNum)
{
  struct sembuf sops;

  sops.sem_num = semNum; // Semaphore number within the semaphore set
  sops.sem_op = 1; // Operation to increment semaphore value
  sops.sem_flg = bsUseSemUndo ? SEM_UNDO : 0; // Use SEM_UNDO if bsUseSemUndo is set

  return semop(semId, &sops, 1); // Perform semaphor operation to release
}
