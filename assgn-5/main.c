/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * Assignment #5
 * (C) 2015 Christian Gunderman
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "monitor.h"

/*
 * NOTES: Please see the README for important info.
 */

// Preprocessor Defines.
#define RAND_SEED time(NULL)   // Change this to a specific seed for debugging
#define START_BALANCE 0.00f;   // The initial balance.

// Constants.
static const int NUM_CHILDREN = 150; // Number of child threads.
static const int A_COND = 0;
static const int B_COND = 1;

// The savings account monitor data fields.
typedef struct savings_account_t {
  float balance;
  float needed;
} savings_account_t;

// Child thread startup params.
typedef struct child_params_t {
  int child_num;
  int usleep_delay;
  int net_transaction;
  monitor_t *monitor;
} child_params_t;

/**
 * Performs a withdrawal from the bank account. 
 * amount is the amount to withdraw as a positive integer and tid
 * is an arbitrary thread id number.
 */
static void withdrawal(monitor_t *monitor, float amount, int tid) {
  // Grab account data from the monitor.
  savings_account_t *account = monitor_data(monitor);

  // Try to enter monitor.
  monitor_enter(monitor);

  // There is somebody waiting for a deposit. Get in line.
  if (account->needed > 0) {
    printf("Thread %i withdrawal of %f waiting on B. Balance %f\n", tid, amount, account->balance);
    monitor_cond_wait(monitor, B_COND);
  }

  // The balance is too small.
  if (account->balance < amount) {
    account->needed = amount - account->balance;
    printf("Thread %i withdrawal of %f waiting on A. Balance %f\n", tid, amount, account->balance);
    monitor_cond_wait(monitor, A_COND);
  }

  // Withdraw our cash.
  account->balance -= amount;
  printf("Thread %i withdrew $%f. Balance %f.\n", tid, amount, account->balance);

  printf("Thread %i signaling B.\n", tid);
  monitor_cond_signal(monitor, B_COND);

  // Leave the monitor and let the next one in.
  monitor_leave(monitor);
}

/**
 * Performs a deposit operation. Amount is the amount to deposit and tid
 * is an arbitrary thread identifier int.
 */
static void deposit(monitor_t *monitor, float amount, int tid) {
  // Grab the account info from the monitor.
  savings_account_t *account = monitor_data(monitor);

  // Try to enter the monitor.
  monitor_enter(monitor);

  // Deposit.
  account->balance += amount;
  printf("Thread %i deposited $%f. Balance %f\n", tid, amount, account->balance);

  // Check if anyone is waiting on a deposit.
  if (account->needed > 0) {

    // If we deposited enough for them, let them in.
    if (amount >= account->needed) {
      account->needed = 0;
      printf("Thread %i signaling A.\n", tid);
      monitor_cond_signal(monitor, A_COND);
    } else {
      account->needed = account->needed - amount;
    }
  }

  monitor_leave(monitor);
}

/**
 * The entry point for the worker threads, a.k.a. bank patrons.
 * input is a pointer to a child_params_t struct.
 */
static void *thread_entry(void *input) {
  // Grab reference and type cast it.
  child_params_t *params = (child_params_t*)input;

  // Pause for randomly delay fed in by main thread.
  // This allows us to simulate the in-out flow of traffic
  printf("Thread %i delaying for %i usec to transact %i\n",
	 params->child_num, params->usleep_delay, params->net_transaction);
  usleep(params->usleep_delay);

  // Perform transaction.
  if (params->net_transaction >= 0) {
    printf("Thread %i trying to deposit $%f...\n",
	   params->child_num, (double)params->net_transaction);
    deposit(params->monitor, params->net_transaction, params->child_num);
  } else {
    printf("Thread %i trying to withdraw $%f...\n",
	   params->child_num, (double)-params->net_transaction);
    withdrawal(params->monitor, -params->net_transaction, params->child_num);
  }

  printf("Thread %i DONE, terminating.\n", params->child_num);

  // Free params structure fed in.
  // This is done so main thread can die in peace at the beginning and not
  // have to wait for all children to die off.
  free(params);
  return NULL;
}

/**
 * Application Entry point. Creates child threads, initializes monitor and
 * semaphores.
 */
int main(int argc, char* argv[]) {

  // Create savings account.
  savings_account_t account;
  account.balance = START_BALANCE;
  account.needed = 0;

  // Create new monitor wrapping the savings account.
  monitor_t *monitor = monitor_create(argv[0], 2, &account, sizeof(savings_account_t));

  // Set random seed to current time.
  srand(RAND_SEED);

  // Make children at random delays.
  int i = 0;
  for (i = 0; i < NUM_CHILDREN; i++) {

    pthread_t new_thread;

    // Allocate params for this thread.
    // Params struct is freed by thread upon thread termination.
    child_params_t *params = malloc(sizeof(child_params_t));

    params->child_num = i;

    // Generate a sleep time between 0 and 2 seconds,
    // non-inclusive (2e6 microseconds) for the child.
    // We do this here so rand() state is preserved. Calling in thread body
    // can lead to repeats since rand() starts at same seed each time.
    params->usleep_delay = rand() % (int)2e6;

    // Generate a random net transaction amount. Negative is withdrawal,
    // positive is deposit. Number generated is between -100 and +200.
    // We do this here so rand() state is preserved. Calling in thread body
    // can lead to repeats since rand() starts at same seed each time.
    params->net_transaction = (rand() % (int)201) - 100;

    // Pass monitor to new thread.
    params->monitor = monitor;

    // Create new child thread.
    if (pthread_create(&new_thread, NULL, thread_entry, params) != 0) {
      perror("Error creating thread.");
      return EXIT_FAILURE;
    }
  }

  // End this thread without terminating child threads.
  // Process auto will end when all children terminate.
  // Children are responsible for freeing their own params structs.
  pthread_exit(NULL);
  
  return EXIT_SUCCESS;
}
