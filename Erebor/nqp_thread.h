#pragma once

typedef struct NQP_THREAD_T nqp_thread_t;
struct thread_control_block;

/**
 * Initialize an nqp_thread_t.
 *
 * Args:
 *  task: the function that this thread should begin executing in. Must not be
 *        NULL.
 *  arg: the arguments to be passed to the function. May be NULL if the function
 *       does not actually use the arguments passed to it.
 * Return: An initialized thread or NULL on error.
 */
nqp_thread_t *nqp_thread_create(void (*task)(void *), void *arg);

/**
 * Wait for the specified thread to finish. This function will block the caller
 * until the specified thread is finished its current task.
 *
 * Args:
 *  thread: the thread to wait for. Must not be NULL. Must have been previously
 *          initialized.
 * Return: 0 on success (e.g., the thread has exited), -1 on failure.
 */
int nqp_thread_join(nqp_thread_t *thread);
