
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
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 0, .exe_time = 510},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 1, .exe_time = 510},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 2, .exe_time = 510},
	{.period = 2000, .subtask_count = 3, .subtask_start_idx = 3, .exe_time = 1800},
	{.period = 4000, .subtask_count = 3, .subtask_start_idx = 6, .exe_time = 2800},
};
struct subtask subtasks[] = {
	{.task_id = 0, .subtask_id = 0, .exe_time = 510, .loop_count = 5075267, .core = 0, .priority = 49, .cumulative_exe_time = 510},
	{.task_id = 1, .subtask_id = 0, .exe_time = 510, .loop_count = 5893716, .core = 2, .priority = 50, .cumulative_exe_time = 510},
	{.task_id = 2, .subtask_id = 0, .exe_time = 510, .loop_count = 5874642, .core = 1, .priority = 50, .cumulative_exe_time = 510},
	{.task_id = 3, .subtask_id = 0, .exe_time = 600, .loop_count = 6933691, .core = 0, .priority = 50, .cumulative_exe_time = 600},
	{.task_id = 3, .subtask_id = 1, .exe_time = 600, .loop_count = 6274416, .core = 2, .priority = 49, .cumulative_exe_time = 1200},
	{.task_id = 3, .subtask_id = 2, .exe_time = 600, .loop_count = 6040600, .core = 1, .priority = 49, .cumulative_exe_time = 1800},
	{.task_id = 4, .subtask_id = 0, .exe_time = 2000, .loop_count = 19921875, .core = 0, .priority = 48, .cumulative_exe_time = 2000},
	{.task_id = 4, .subtask_id = 1, .exe_time = 400, .loop_count = 3963097, .core = 2, .priority = 48, .cumulative_exe_time = 2400},
	{.task_id = 4, .subtask_id = 2, .exe_time = 400, .loop_count = 3947553, .core = 1, .priority = 48, .cumulative_exe_time = 2800},
};
