#pragma once

#include <unistd.h>
#include <stdint.h>
#include "nqp_thread.h"

typedef enum NQP_SCHEDULING_POLICY
{
    NQP_SP_TWOTHREADS, // this is for testing swapping between two threads. this
                       // is the default scheduling option (this is what's set
                       // if nqp_sched_init is not called).
                       
    NQP_SP_FIFO,       // FIFO scheduling, yield should reschedule the current
                       // task until it is fully complete (it calls
                       // nqp_exit()); this policy will not work with tasks that
                       // attempt to acquire locks (lock acquisition would 
                       // result in the same task always being scheduled).

    NQP_SP_RR,         // RR scheduling, yield should schedule the next task in
                       // the queue.

    NQP_SP_MLFQ,       // MLFQ scheduling, yield should schedule the next task
                       // in the queue using MLFQ rules. When passed to
                       // nqp_sched_init, settings must not be NULL and must be
                       // an instance of nqp_sp_settings with the mlfq_settings
                       // field populated.

    NQP_SP_POLICIES    // Number of policies; this should always be last.
} nqp_scheduling_policy;

typedef struct NQP_SP_MLFQ_SETTINGS 
{
    useconds_t queue_time_allotment; // this is time allotment as described in 
                                     // OSTEP for the final set of MLFQ rules.

    useconds_t boost_time;           // this is S as desribed in OSTEP for the
                                     // final set of MLFQ rules.

    uint8_t queues;                  // how many queues should we have?
} nqp_sp_mlfq_settings;

typedef union NQP_SP_SETTINGS
{
    nqp_sp_mlfq_settings mlfq_settings;
} nqp_sp_settings;

/**
 * The policy that should be used when deciding which task to switch to next 
 * upon a call to yield.
 *
 * Args:
 *  policy: the policy to apply. Must be a policy as defined in 
 *          NQP_SCHEDULING_POLICY.
 *  settings: The settings for the policy to apply. May be NULL, depending on
 *            the policy being set. See nqp_scheduling_policy.
 * Returns: 0 on success, -1 on failure (e.g., the specified policy is not in 
 *          NQP_SCHEDULING_POLICY).
 */
int nqp_sched_init( const nqp_scheduling_policy policy, 
                    const nqp_sp_settings *settings );

/**
 * Start scheduling tasks.
 *
 * If the NQP_SP_TWOTHREADS policy is selected, control flow will never return
 * to the caller (the program must be terminated by sending the SIGINT signal).
 *
 * If any other scheduling policy is selected, control flow will eventually
 * return to the caller. The caller is then responsible for blocking itself
 * until all threads of execution that it has started have finished by calling
 * nqp_thread_join for each thread it has started.
 */
void nqp_sched_start( void );

/**
 * Voluntarily give up control of the processor and allow the scheduler to
 * schedule another task.
 *
 * When called outside of the context of an NQP thread (e.g., nqp_thread_start
 * has not been called), this function should have no side-effects at all (the
 * function should behave as a no-op).
 */
void nqp_yield( void );

/**
 * The current task is done, it should be removed from the queue of tasks to
 * be scheduled.
 *
 * When called outside of the context of an NQP thread (e.g., nqp_thread_start
 * has not been called), this function should have no side-effects at all (the
 * function should behave as a no-op).
 */
void nqp_exit( void );
