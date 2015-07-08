/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Preprocessor Defines.
#define NUM_CHILDREN 70
#define SEM_PROJ_ID  1
#define MUTEX_SEM 0
#define WEST_BOUND_SEM 1
#define EAST_BOUND_SEM 2
#define RAND_SEED time(NULL)

// Constants.
static const char *SHM_NAME = "/EECS338ProjectSharedMem";

// Global state:
// Should be avoided at all costs, but is neccessary in this case to allow for
// atexit handler to kill remaining semaphores at exit because System V doesn't
// do this by default and creation fails next execution.
static int g_sem_id = -1;
static int g_mem_id = -1;

typedef enum direction_t {
  NONE = 0,
  EAST_BOUND = 1,
  WEST_BOUND = 2
} direction_t;

// Shared memory data.
typedef struct shared_data_t {
  int crossing_count;
  int crossed_count;
  int east_bound_wait_count;
  int west_bound_wait_count;
  direction_t crossing_direction;
} shared_data_t;

/**
 * Creates a new semaphore set with the specified number of semaphores
 * and the given default values and stores the set id in global state.
 */
static void semaphore_create(char *app_name, int num, int *values) {
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
  g_sem_id = semget(sem_key, num, IPC_CREAT | IPC_EXCL | S_IRUSR
                    | S_IWUSR | S_IROTH | S_IWOTH);

  // Create error.
  if (g_sem_id == -1) {
    printf("PID: %i, PARENT: Error creating semaphores.\n",
           getpid());
    perror("Semaphore error");
    exit(EXIT_FAILURE);
  }

  // Set default semaphore values.
  if (semctl(g_sem_id, /* Ignored. */ 0, SETALL, values) == -1) {
    printf("PID: %i, PARENT: Error initializing semaphores.\n",
           getpid());
    perror("Semaphores initialization error");
    exit(EXIT_FAILURE);
  }
}

/**
 * Waits on a semaphore. num is a semaphore id 
 * from the DEFINES at the top of the module.
 */
static void semaphore_wait(int num) {
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

  if (semop(g_sem_id, decrement_sops, 1) != 0 ||
      semop(g_sem_id, wait_sops, 1) != 0) {
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
static void semaphore_signal(int num) {
  const unsigned nsops = 1;
  struct sembuf sops[nsops];

  // Increment.
  sops[0].sem_num = num;
  sops[0].sem_op = 1;
  sops[0].sem_flg = 0;

  if (semop(g_sem_id, sops, nsops) != 0) {
    printf("PID: %i, CHILD: Error signaling in semaphore.\n",
           getpid());
    perror("Semaphore signal error");
    exit(EXIT_FAILURE);
  }
}

/**
 * Deletes the semaphore. If we don't do this, System V doesn't do it for us
 * until reboot.
 */
static void semaphore_delete(int sem_id) {
  if (semctl(sem_id, /* Ignored. */ 0, IPC_RMID) == -1) {
    printf("PID: %i, PARENT: Error deleting semaphores.\n",
           getpid());
  }
}

/**
 * East bound process algorith, adapted from the pseudo code
 * provided by prof. shared_mem is a reference to the shared memory
 * structure. This should only be accessed during the critical sections.
 */
static void east_bound_process(shared_data_t *shared_mem) {
  printf("PID: %i, EASTBOUND: waiting for mutex to get in line.\n", getpid());
  semaphore_wait(MUTEX_SEM);

  if ((shared_mem->crossing_direction == EAST_BOUND ||
       shared_mem->crossing_direction == NONE) &&
      shared_mem->crossing_count < 4 &&
      (shared_mem->crossed_count + shared_mem->crossing_count) < 5) {

    shared_mem->crossing_direction = EAST_BOUND;
    shared_mem->crossing_count++;
    semaphore_signal(MUTEX_SEM);
  } else {
    shared_mem->east_bound_wait_count++;

    semaphore_signal(MUTEX_SEM);

    printf("PID: %i, EASTBOUND: waiting for east bound semaphore.\n", getpid());
    semaphore_wait(EAST_BOUND_SEM);
    shared_mem->east_bound_wait_count--;
    shared_mem->crossing_count++;
    shared_mem->crossing_direction = EAST_BOUND;
    semaphore_signal(MUTEX_SEM);
  }

  printf("PID: %i, EASTBOUND: crossing.\n", getpid());

  printf("PID: %i, EASTBOUND: waiting for mutex to get off bridge.\n", getpid());
  semaphore_wait(MUTEX_SEM);
  shared_mem->crossed_count++;
  shared_mem->crossing_count--;

  if (shared_mem->east_bound_wait_count != 0 &&
      ((shared_mem->crossed_count + shared_mem->crossing_count) < 5 ||
       shared_mem->west_bound_wait_count == 0)) {
    semaphore_signal(EAST_BOUND_SEM);
  } else if (shared_mem->crossing_count == 0 &&
             shared_mem->west_bound_wait_count != 0 &&
             (shared_mem->east_bound_wait_count == 0 || 
              (shared_mem->crossed_count + shared_mem->crossing_count) >= 5)) {
    shared_mem->crossing_direction = WEST_BOUND;
    shared_mem->crossed_count = 0;
    semaphore_signal(WEST_BOUND_SEM);
  } else if (shared_mem->crossing_count == 0 &&
             shared_mem->east_bound_wait_count == 0 &&
             shared_mem->west_bound_wait_count == 0) {
    shared_mem->crossing_direction = NONE;
    shared_mem->crossed_count = 0;
    semaphore_signal(MUTEX_SEM);
  } else {
    semaphore_signal(MUTEX_SEM);
  }
  printf("PID: %i, EASTBOUND: off bridge.\n", getpid());
}

/**
 * West bound process algorith, adapted from the pseudo code
 * provided by prof. shared_mem is a reference to the shared memory
 * structure. This should only be accessed during the critical sections.
 */
static void west_bound_process(shared_data_t *shared_mem) {
  printf("PID: %i, WESTBOUND: waiting for mutex to get in line.\n", getpid());
  semaphore_wait(MUTEX_SEM);

  if ((shared_mem->crossing_direction == WEST_BOUND ||
       shared_mem->crossing_direction == NONE) &&
      shared_mem->crossing_count < 4 &&
      (shared_mem->crossed_count + shared_mem->crossing_count) < 5) {
    shared_mem->crossing_direction = WEST_BOUND;
    shared_mem->crossing_count++;
    semaphore_signal(MUTEX_SEM);
  } else {
    shared_mem->west_bound_wait_count++;

    semaphore_signal(MUTEX_SEM);

    printf("PID: %i, WESTBOUND: waiting for east bound semaphore.\n", getpid());
    semaphore_wait(WEST_BOUND_SEM);
    shared_mem->west_bound_wait_count--;
    shared_mem->crossing_count++;
    shared_mem->crossing_direction = WEST_BOUND;
    semaphore_signal(MUTEX_SEM);
  }

  printf("PID: %i, WESTBOUND: crossing..\n", getpid());

  printf("PID: %i, WESTBOUND: waiting for mutex to get off bridge.\n", getpid());
  semaphore_wait(MUTEX_SEM);
  shared_mem->crossed_count++;
  shared_mem->crossing_count--;

  if (shared_mem->west_bound_wait_count != 0 &&
      ((shared_mem->crossed_count + shared_mem->crossing_count) < 5 ||
       shared_mem->east_bound_wait_count == 0)) {
    semaphore_signal(WEST_BOUND_SEM);
  } else if (shared_mem->crossing_count == 0 &&
             shared_mem->east_bound_wait_count != 0 &&
             (shared_mem->west_bound_wait_count == 0 ||
              (shared_mem->crossed_count + shared_mem->crossing_count) >= 5)) {
    shared_mem->crossing_direction = EAST_BOUND;
    shared_mem->crossed_count = 0;
    semaphore_signal(EAST_BOUND_SEM);
  } else if (shared_mem->crossing_count == 0 &&
             shared_mem->east_bound_wait_count == 0 &&
             shared_mem->west_bound_wait_count == 0) {
    shared_mem->crossing_direction = NONE;
    shared_mem->crossed_count = 0;
    semaphore_signal(MUTEX_SEM);
  } else {
    semaphore_signal(MUTEX_SEM);
  }
  printf("PID: %i, WESTBOUND: off bridge.\n", getpid());
}

/**
 * Opens a shared memory pointer to a shared_data_t struct containing the
 * algorithms variables.
 */
static shared_data_t *sharedmem_open() {

  // Try to open.
  g_mem_id = shm_open(SHM_NAME, O_CREAT | O_RDWR,
                        S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);

  // Handle errors if any.
  if (g_mem_id == -1) {
    printf("PID: %i, PARENT: Unable to create shared memory.\n", getpid());
    perror("Shared memory error");
    exit(EXIT_FAILURE);
  }

  // Expand the open shared memory to the size of our shared_data_t struct.
  ftruncate(g_mem_id, sizeof(shared_data_t));

  shared_data_t *data_addr = mmap(NULL, sizeof(shared_data_t),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, g_mem_id, 0);
  // Check for mmap error.
  if (data_addr == NULL) {
    printf("PID: %i, PARENT: Unable to map shared variables.\n", getpid());
    perror("Shared memory error");
    exit(EXIT_FAILURE);
  }

  printf("PID: %i: Created sharedmem, id: %i.\n", getpid(), g_mem_id);
  return data_addr;
}

/**
 * Creates an initializes a shared memory block for shared_data_t.
 */
static shared_data_t *sharedmem_create() {
  shared_data_t *shared_mem = sharedmem_open();

  // Clear memory to zero.
  // Prevents garbage.
  memset(shared_mem, 0, sizeof(shared_data_t));

  return shared_mem;
}

/**
 * Deletes a shared memory block if it was allocated.
 */
static void sharedmem_delete() {
  if (shm_unlink(SHM_NAME) == -1) {
    printf("PID: %i, PARENT: Unable to delete shared memory.\n", getpid());
    perror("Shared memory error");
    exit(EXIT_FAILURE);
  }
}

/**
 * At exit handler that disposes of semaphores and shared memory at shutdown
 * if they have been allocated.
 */
static void at_exit_handler() {
  if (g_sem_id != -1) {
    semaphore_delete(g_sem_id);
    printf("PID: %i, PARENT: Deleted semaphores, id: %i.\n", getpid(), g_sem_id);
  }

  if (g_mem_id != -1) {
    sharedmem_delete();
    printf("PID: %i, PARENT: Deleted shared mem, id: %i.\n", getpid(), g_mem_id);
  }    
}

/**
 * Creates num_children new children, assigning them to east or west randomly with
 * a seed based upon the current time.
 */
static void fork_children(pid_t *pid, int num_children, shared_data_t *data) {

  int i = 0;
  int random = rand();
  for (i = 0; i < num_children; i++) {
    pid_t fork_result = fork();

    if (fork_result == -1) {
      // Failure forking.
      printf("PID: %i, PARENT: Failure during fork.\n", getpid());
      perror("Fork error");
      exit(EXIT_FAILURE);
    } else if (fork_result == 0) {
      // Child fork.
      printf("PID: %i, CHILD: New Child.\n", getpid());
      if (random % 2 == 0) {
        east_bound_process(data);
      } else {
        west_bound_process(data);
      }
      exit(EXIT_SUCCESS);
    } else {
      // Parent fork.
      pid[i] = fork_result;

      // This has to be done here or else we will get the same random number
      // EVERY time because all processes are forking at the same place,
      // calling once, and share the same seed.
      random = rand();
    }
  }

  // Success, Parent.
  printf("PID: %i, PARENT: Parent process.\n", getpid());

  // Create at exit handler to clean up semaphore.
  if (atexit(at_exit_handler) != 0) {
    printf("PID: %i, PARENT: Error setting semaphore cleanup handler.", getpid());
    exit(EXIT_FAILURE);
  }

  // Wait a few seconds to let things finish.
  // I tried to use wait() but for some reason it was only waiting for a few children,
  // not all and I had no time left to debug.
  printf("PID: %i, PARENT: Parent waiting 3 seconds...\n", getpid());
  sleep(3);
  printf("PID: %i, PARENT: Parent cleanup and terminate.\n", getpid());
}

/**
 * Application Entry point.
 */
int main(int argc, char* argv[]) {
  pid_t children[NUM_CHILDREN];
  int sem_values[3] = { 1, 0, 0 };

  // Get a pointer to a shared data struct.
  shared_data_t *data = sharedmem_create();

  // Set time based rando seed.
  srand(RAND_SEED);

  // Create semaphore.
  semaphore_create(argv[0], 3, sem_values);
  printf("PID: %i, PARENT: Created semaphore, id: %i.\n", getpid(), g_sem_id);

  // Create children.
  printf("PID: %i, PARENT: Forking children.\n",
         getpid()); 
  fork_children(children, NUM_CHILDREN, data);

  return EXIT_SUCCESS;
}

