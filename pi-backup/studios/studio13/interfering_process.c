#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <assert.h>

const int num_expected_args = 6;

void
task_function(int iterNum)
{
    int i;
    for (i = 0; i < iterNum; ++i)
    {
        int tmp = i + 128 * 512;
    }
}

int
main(int argc, char const *argv[])
{
    int policy, priority, coreNum, period, iterNum;
    struct sched_param scheduler;
    cpu_set_t cpu_set;

    if ( argc < num_expected_args )
    {
        printf("Usage: ./interfering_process <type of scheduler> <priority> <# of the core> <period(ms)> <# of iteration>\n");
        return -1;
    }
    if (!strcmp(argv[1], "OTHER"))  policy = SCHED_OTHER;
    else if (!strcmp(argv[1], "FIFO"))  policy = SCHED_FIFO;
    else if (!strcmp(argv[1], "RR"))  policy = SCHED_RR;
    else
    {
        printf("Error: the policy should be one of [OTHER, FIFO, RR].");
        return -1;
    }

    priority = atoi(argv[2]);
    coreNum = atoi(argv[3]);
    period = atoi(argv[4]);
    iterNum = atoi(argv[5]);

    if (priority < sched_get_priority_min(policy) || priority > sched_get_priority_max(policy))
    {
        printf("Error: Bad priority!\n");
        return -1;
    }
 
    scheduler.sched_priority = priority;
    if (sched_setscheduler(0, policy, &scheduler) == -1)
    {
        printf("Error, set policy fail! the policy is %d, and the priority is %d\n", policy, scheduler.sched_priority);
        return -1;
    }

    CPU_ZERO(&cpu_set);
    CPU_SET(coreNum, &cpu_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

    while(1)
    {
        sleep((float) period / 1000);
        task_function(iterNum);
    }



    return 0;
}
