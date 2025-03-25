#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h> // Make sure to include this for write(), usleep()
#include "nqp_thread.h"
#include "nqp_thread_sched.h"

#define MAX_PROGRESS 30
#define THREADS 40

typedef enum THREAD_TYPE
{
    INTERACTIVE,
    SEMI_INTERACTIVE,
    NON_INTERACTIVE,

    THREAD_TYPES
} thread_type;

typedef struct THREAD
{
    uint64_t progress;
    thread_type type;
} thread;

useconds_t work_time[] =
    {
        [INTERACTIVE] = 1000,
        [SEMI_INTERACTIVE] = 4000,
        [NON_INTERACTIVE] = 10000,
};

char work_symbol[] =
    {
        [INTERACTIVE] = 'i',
        [SEMI_INTERACTIVE] = 's',
        [NON_INTERACTIVE] = 'n',
};

void do_work(void *arg)
{
    thread *t = (thread *)arg;
    assert(t != NULL);
    assert(t->type >= 0 && t->type < THREAD_TYPES);

    while (t->progress < MAX_PROGRESS)
    {
        t->progress++;
        (void)write(STDOUT_FILENO, &work_symbol[t->type], 1);
        usleep(work_time[t->type]);
        nqp_yield();
    }
    nqp_exit();
}

int main(void)
{
    thread threads[THREADS] = {0};
    nqp_thread_t *nqp_threads[THREADS] = {0};

    for (int i = 0; i < THREADS; i++)
    {
        threads[i].progress = 0;
        // one third will be "interactive", one third "semi-interactive", and
        // one third "noninteractive".
        if (i < THREADS / 3)
        {
            (void)write(STDOUT_FILENO, &work_symbol[INTERACTIVE], 1);
            threads[i].type = INTERACTIVE;
        }
        else if (i >= THREADS / 3 && i < (2 * THREADS / 3))
        {
            (void)write(STDOUT_FILENO, &work_symbol[SEMI_INTERACTIVE], 1);
            threads[i].type = SEMI_INTERACTIVE;
        }
        else
        {
            (void)write(STDOUT_FILENO, &work_symbol[NON_INTERACTIVE], 1);
            threads[i].type = NON_INTERACTIVE;
        }

        nqp_threads[i] = nqp_thread_create(do_work, &threads[i]);
    }

    (void)write(STDOUT_FILENO, "\n", 1);

    nqp_sp_settings settings = {0};
    settings.mlfq_settings.queue_time_allotment = 125000;
    settings.mlfq_settings.boost_time = 2000000;

    nqp_sched_init(NQP_SP_MLFQ, &settings);

    // you can uncomment these to see how the behaviour of threads changes with
    // the scheduling policies you've implemented:
    //    nqp_sched_init( NQP_SP_RR, NULL );
    //    nqp_sched_init( NQP_SP_FIFO, NULL );
    nqp_sched_start();

    for (int i = 0; i < THREADS; i++)
    {
        nqp_thread_join(nqp_threads[i]);
    }

    (void)write(STDOUT_FILENO, "\nAll done!\n", 11);

    return EXIT_SUCCESS;
}
