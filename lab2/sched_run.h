
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/kthread.h>

struct task {
    uint32_t period;
    uint32_t subtask_count;
    uint32_t subtask_start_idx;
    uint32_t exe_time; 
};

struct subtask {
    uint32_t task_id;
    uint32_t subtask_id;
    uint32_t exe_time;
    struct hrtimer timer;
    struct task_struct *thread;
    ktime_t last_release_time;
    uint32_t loop_count;
    uint32_t cumulative_exe_time;
    uint32_t core;
    uint32_t priority;
};

struct task tasks[] = {
{.period = 6000, .subtask_count = 3, .subtask_start_idx = 0},
{.period = 2000, .subtask_count = 3, .subtask_start_idx = 3},
};
struct subtask subtasks[] = {
{.task_id = 0, .subtask_id = 0, .loop_count = 11518556, .core = 1, .priority = 49},
{.task_id = 0, .subtask_id = 1, .loop_count = 23110352, .core = 0, .priority = 49},
{.task_id = 0, .subtask_id = 2, .loop_count = 23110352, .core = 0, .priority = 48},
{.task_id = 1, .subtask_id = 0, .loop_count = 5764161, .core = 1, .priority = 50},
{.task_id = 1, .subtask_id = 1, .loop_count = 5764161, .core = 1, .priority = 48},
{.task_id = 1, .subtask_id = 2, .loop_count = 5771485, .core = 0, .priority = 50},
};
