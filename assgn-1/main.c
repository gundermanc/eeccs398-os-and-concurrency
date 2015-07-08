/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */
#include <errno.h>
#include <pwd.h>
#include <linux/limits.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/times.h>

// Preprocessor Defines.
#define INIT_DELAY 0.5  // sec.
#define PERIODIC_DELAY 1 // sec.
#define MAIN_ITERATIONS 10 // Fib. seq. iterations
#define CWD_MAX PATH_MAX // chars.

// Forward Decls.
static void fork_children();
static bool fork_child(bool second_child);
static void fork_error();
static void child_process(bool second_child);
static void parent_process();
static void try_sleep(int sleep_time);
static void print_user_info(const char* process);
static void print_time_stats(const char* process);

/**
 * Application Entry point
 */
int main(int argc, char* argv[]) {
  char working_dir[CWD_MAX];

  // Print current working directory and user info.
  if (getcwd(working_dir, CWD_MAX) == NULL) {
    perror("Failure obtaining current working directory.");
  }
  printf("PID %i, PARENT: Current working directory: %s\n", getpid(), working_dir);
  print_user_info("PARENT");

  // Fork Children.
  printf("PID %i, PARENT: This is the parent process.\n", getpid());
  fork_children();

  return EXIT_SUCCESS;
}

/**
 * Forks parent into a parent and 2 child processes.
 */
static void fork_children() {

  // If true, this is the parent.
  if (fork_child(false)) {
    // Fork a second time.
    if (fork_child(true)) {
      parent_process();
    }
  }
}

/**
 * Forks the process into a parent and child and starts the child
 * process main function.
 */
static bool fork_child(bool second_child) {
  printf("PID %i, PARENT: Forking child.\n", getpid());
  pid_t pid = fork();

  // Check that fork was successful.
  if (pid == -1) {
    fork_error();
    return false;
  } else if (pid == 0) {
    print_user_info("CHILD");
    child_process(second_child);
    return false;
  } else {
    return true;
  }
}

/**
 * Error occurred while forking process.
 */
static void fork_error() {
  perror("Error forking process");
  exit(EXIT_FAILURE);
}

/**
 * Main function of child processes. If second_child is true,
 * process offsets fib. sequence by one so that every other
 * number is produced by each of the children.
 */
static void child_process(bool second_child) {
  int first_number = 0;
  int second_number = 1;
  int i = 0;

  printf("PID %i, CHILD: This is a child process.\n", getpid());

  // Offset the second child by a second so that order is maintained.
  if (second_child) {
    try_sleep(INIT_DELAY);
  }

  // Alternating Fib sequence.
  for (i = 0; i < MAIN_ITERATIONS; i++) {

    // Calculate next number and print.
    int next_number = first_number + second_number;

    // Print every other one, depending on first or second child.
    if ((second_child && (i % 2 == 0)) ||
    	(!second_child && (i % 2 != 0))) {
      printf("PID %i, CHILD: Fib(%i) = %i\n", getpid(), i, first_number);
    }

    // Update prior two numbers.
    first_number = second_number;
    second_number = next_number;

    // Pause to let other child go.
    sleep(PERIODIC_DELAY);
  }

  print_time_stats("CHILD");

  // Print exit message.
  printf("PID %i, CHILD: %s Child Terminating.\n",
	 getpid(), second_child ? "First" : "Second");
  exit(EXIT_SUCCESS);
}

/**
 * Evaluates a wait() status integer.
 */
static void evaluate_status(pid_t pid, int status) {
  if (WIFEXITED(status)) {
    printf("Child process %i exited normally, code: %i.\n", pid, WEXITSTATUS(status));
  } else {
    printf("Child process %i did not exit normally!\n", pid);
  }
}

/**
 * Main function for parent process, post fork().
 */
static void parent_process() {
  int status_1;
  int status_2;
  pid_t pid_1;
  pid_t pid_2;

  // Wait for both children to end:
  if ((pid_1 = wait(&status_1)) == -1) {
    perror("Error waiting for child to terminate.");
  }
  evaluate_status(pid_1, status_1);

  if ((pid_2 = wait(&status_2)) == -1) {
    perror("Error waiting for child to terminate.");
  }
  evaluate_status(pid_2, status_2);

  print_time_stats("PARENT");
  printf("PID %i, PARENT: Parent terminating.\n", getpid());
  exit(EXIT_SUCCESS);
}

/**
 * Try to sleep the current process. If interrupted, give error message
 * and quit.
 */
static void try_sleep(int sleep_time) {
  if (sleep(sleep_time) != 0) {
    printf("PID %i, CHILD: Sleep Interrupted! Exiting.", getpid());
    exit(EXIT_FAILURE);
  }
}

/**
 * Print process user information to the console.
 */
static void print_user_info(const char* process) {

  // MAN pages say that cuserid() is deprecated, so I am using the passwd
  // structure to get the user name instead.
  const struct passwd* user_info = getpwuid(getuid());
  if (user_info == NULL) {
    perror("Unable to get user info.");
    exit(EXIT_FAILURE);
  }

  printf("PID %i, %s: Running as username: %s\n", getpid(), process, user_info->pw_name);
  printf("PID %i, %s: With real ID: %i\n", getpid(), process, getuid());
  printf("PID %i, %s: With effective ID: %i\n", getpid(), process, geteuid());
  printf("PID %i, %s: With group ID: %i\n", getpid(), process, getgid());
}

/**
 * Loop for a bit to waste user CPU time.
 */
static void waste_cpu_time(const char* process) {
  int i = 0;

  // Just loop a bunch to use up user CPU cycles so clock has something
  // to show.
  printf("PID %i, %s: Wasting CPU time, please be patient...\n", getpid(), process);
  for (i = 0; i < INT_MAX; i++);
}

/**
 * Prints current system time, cpu time used, and system CPU time used
 * by/on behalf of this process.
 */
static void print_time_stats(const char* process) {
  const time_t current_time = time(NULL);
  struct tms cpu_times;
  long clock_ticks = sysconf(_SC_CLK_TCK);

  // Waste a bunch of time.
  waste_cpu_time(process);

  // Get CPU times.
  if (times(&cpu_times) == (clock_t)-1) {
    perror("Unable to obtain CPU times.");
  }

  double user_time = cpu_times.tms_utime / (double)clock_ticks;
  double sys_time = cpu_times.tms_stime / (double)clock_ticks;

  printf("PID %i, %s: Current time: %s", getpid(), process, ctime(&current_time));
  printf("PID %i, %s: Approx. User CPU Time (s): %f\n", getpid(), process, user_time);
  printf("PID %i, %s: Approx. System CPU Time (s): %f\n", getpid(), process, sys_time);
}
