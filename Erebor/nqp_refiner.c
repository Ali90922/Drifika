#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "nqp_thread.h"
#include "nqp_thread_sched.h"

#define THREADS 40
#define MAX_PROGRESS 30
#define MAX_WORK_UNIT 3

uint8_t progress[THREADS] = {0};

bool all_full(void)
{
    bool full = true;

    for (int i = 0; i < THREADS; i++)
    {
        full &= progress[i] >= MAX_PROGRESS;
    }
    return full;
}
void refine_macrodata(void *arg)
{
    assert(arg != NULL);
    size_t my_bar = *(int *)arg;
    assert(my_bar >= 0);
    assert(my_bar < THREADS);

    while (progress[my_bar] < MAX_PROGRESS)
    {
        progress[my_bar] += rand() % MAX_WORK_UNIT;

        if (progress[my_bar] > MAX_PROGRESS)
        {
            progress[my_bar] = MAX_PROGRESS;
        }
        nqp_yield();
    }
    nqp_exit();
}

void display_progress(void)
{
    // clear the screen
    printf("\033[2J");

    for (int i = 0; i < THREADS; i++)
    {
        printf("Refiner %02d: ", i);
        if (progress[i] >= MAX_PROGRESS)
        {
            // green checkmark
            printf("\U00002705 ");
        }
        else
        {
            // hourglass with flowing sand
            printf("\U000023F3 ");
        }
        for (int j = 0; j < progress[i]; j++)
        {
            putchar('|');
        }
        putchar('\n');
    }
}

void grapher(void *arg)
{
    (void)arg;
    // print a graph until all the bars are full.
    while (!all_full())
    {
        display_progress();
        usleep(125000);
        nqp_yield();
    }
    // display final progress where all the macrodata is refined.
    display_progress();
    nqp_exit();
}

int main(void)
{
    int work_unit[THREADS];
    nqp_thread_t *display = nqp_thread_create(grapher, NULL);
    nqp_thread_t *macrodata_refiners[THREADS] = {0};

    for (int i = 0; i < THREADS; i++)
    {
        work_unit[i] = i;
        macrodata_refiners[i] = nqp_thread_create(refine_macrodata, &work_unit[i]);
    }

    // this can't work with FIFO, all threads need to be able to make progress
    // for the display to work.

    // nqp_sched_init(NQP_SP_RR, NULL);
    nqp_sched_init(NQP_SP_FIFO, NULL);
    nqp_sched_start();

    nqp_thread_join(display);
    for (int i = 0; i < THREADS; i++)
    {
        nqp_thread_join(macrodata_refiners[i]);
    }

    printf("All done!\n");

    return EXIT_SUCCESS;
}
