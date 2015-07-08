/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "monitor.h"

/*
 * My implementation of generic monitor facilities.
 */

// Constants.
static const int SEM_PROJ_ID = 1;
static const int MUTEX_SEM = 0;
static const int NEXT_SEM = 0;
static const int MONITOR_SEM_COUNT = 2;

/**
 * Creates a new semaphore set with the specified number of semaphores
 * and the given default values and stores the set id in global state.
 */
static int semaphore_create(char *app_name, int num, int *values) {
  // Create semaphore key.
  key_t sem_key = ftok(app_name, SEM_PROJ_ID);

  // Error occurred.
  if (sem_key == -1) {
    printf("PID: %i, PARENT: Error creating semaphore key.\n",
           getpid());
    perror("Semaphore key error");
    exit(EXIT_FAILURE);
  }

  // Create new Semaphore.
  int sem_id = semget(sem_key, num, IPC_CREAT | S_IRUSR
                    | S_IWUSR | S_IROTH | S_IWOTH);

  // Create error.
  if (sem_id == -1) {
    printf("PID: %i, PARENT: Error creating semaphores.\n",
           getpid());
    perror("Semaphore error");
    exit(EXIT_FAILURE);
  }

  // Set default semaphore values.
  if (semctl(sem_id, /* Ignored. */ 0, SETALL, values) == -1) {
    printf("PID: %i, PARENT: Error initializing semaphores.\n",
           getpid());
    perror("Semaphores initialization error");
    exit(EXIT_FAILURE);
  }

  return sem_id;
}

/**
 * Waits on a semaphore. num is a semaphore id 
 * from the DEFINES at the top of the module.
 */
static void semaphore_wait(int sem_id, int num) {
  struct sembuf decrement_sops[1];
  struct sembuf wait_sops[1];

  // Decrement.
  decrement_sops[0].sem_num = num;
  decrement_sops[0].sem_op = -1;
  decrement_sops[0].sem_flg = 0;

  // Wait for zero.
  wait_sops[0].sem_num = num;
  wait_sops[0].sem_op = 0;
  wait_sops[0].sem_flg = 0;

  if (semop(sem_id, decrement_sops, 1) != 0 ||
      semop(sem_id, wait_sops, 1) != 0) {
    printf("PID: %i, CHILD: Error waiting in semaphore.\n",
           getpid());
    perror("Semaphore wait error");
    exit(EXIT_FAILURE);
  }
}

/**
 * Signals a semaphore. num is a semaphore id 
 * from the DEFINES at the top of the module.
 */
static void semaphore_signal(int sem_id, int num) {
  const unsigned nsops = 1;
  struct sembuf sops[nsops];

  // Increment.
  sops[0].sem_num = num;
  sops[0].sem_op = 1;
  sops[0].sem_flg = 0;

  if (semop(sem_id, sops, nsops) != 0) {
    printf("PID: %i, CHILD: Error signaling in semaphore.\n",
           getpid());
    perror("Semaphore signal error");
    exit(EXIT_FAILURE);
  }
}

/**
 * Deletes the semaphore. If we don't do this, System V doesn't do it for us
 * until reboot and semaphore creation fails next execution.
 */
static void semaphore_delete(int sem_id) {
  if (semctl(sem_id, /* Ignored. */ 0, IPC_RMID) == -1) {
    printf("PID: %i, PARENT: Error deleting semaphores.\n",
           getpid());
  }
}

/**
 * Creates a new monitor. App name is the application to create a monitor for,
 * cond_count is the number of conditional variables, data is the struct that
 * this monitor encapsulates, and size is the size of the data struct.
 */
monitor_t *monitor_create(char *app_name, int cond_count, 
			  void *data, size_t size) {
  monitor_t *monitor = malloc(sizeof(monitor_t));
  int values[] = {1, 0};

  // Create semaphore set for this set of monitors.
  monitor->sem_id
    = semaphore_create(app_name, MONITOR_SEM_COUNT + cond_count, values);

  // Init other fields.
  monitor->next_count = 0;
  monitor->x_count = calloc(cond_count, sizeof(int));

  // Store user provided monitor data structures.
  if (data != NULL) {
    monitor->data = malloc(size);
    memcpy(monitor->data, data, size);
  }

  return monitor;
}

/**
 * Deletes a monitor object and frees associated memory.
 */
void monitor_delete(monitor_t *monitor) {
  semaphore_delete(monitor->sem_id);
  free(monitor);
}

/**
 * Placed at the head of all monitor functions, this enters a monitor
 * or waits if it is occupied.
 */
void monitor_enter(monitor_t *monitor) {
  semaphore_wait(monitor->sem_id, MUTEX_SEM);
}

/**
 * Placed at the end of all monitor functions, this leaves a monitor.
 */
void monitor_leave(monitor_t *monitor) {
  if (monitor->next_count > 0) {
    semaphore_signal(monitor->sem_id, NEXT_SEM);
  } else {
    semaphore_signal(monitor->sem_id, MUTEX_SEM);
  }
}

/**
 * Wrapper function. Accesses monitor's internal data struct.
 */
void *monitor_data(monitor_t *monitor) {
  return monitor->data;
}

/**
 * Waits on the specified condition variable in the monitor.
 * cond is the zero based index of the condition variable which
 * must be less than the number of condition variables the monitor
 * was created with.
 */
void monitor_cond_wait(monitor_t *monitor, int cond) {
  monitor->x_count[cond]++;

  if (monitor->next_count > 0) {
    semaphore_signal(monitor->sem_id, NEXT_SEM);
  } else {
    semaphore_signal(monitor->sem_id, MUTEX_SEM);
  }

  semaphore_wait(monitor->sem_id, MONITOR_SEM_COUNT + cond);
  monitor->x_count[cond]--;
}

/**
 * Signals a monitor condition variable.
 * cond is the zero based index of the condition variable which
 * must be less than the number of condition variables the monitor
 * was created with.
 */
void monitor_cond_signal(monitor_t *monitor, int cond) {
  if (monitor->x_count[cond] > 0) {
    monitor->next_count++;
    semaphore_signal(monitor->sem_id, MONITOR_SEM_COUNT + cond);
    semaphore_wait(monitor->sem_id, NEXT_SEM);
    monitor->next_count--;
  }
}
