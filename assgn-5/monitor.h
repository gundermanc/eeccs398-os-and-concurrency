/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */
#ifndef MONITOR__H__
#define MONITOR__H__

typedef struct monitor_t {
  int sem_id;
  int next_count;
  int *x_count;
  void *data;
} monitor_t;

monitor_t *monitor_create(char *app_name, int num_cond,
			  void *data, size_t size);

void monitor_delete(monitor_t *monitor);

void monitor_enter(monitor_t *monitor);

void monitor_leave(monitor_t *monitor);

void *monitor_data(monitor_t *monitor);

void monitor_cond_wait(monitor_t *monitor, int cond);

void monitor_cond_signal(monitor_t *monitor, int cond);

#endif // MONITOR__H__
