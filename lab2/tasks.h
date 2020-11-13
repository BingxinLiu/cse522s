#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/kthread.h>

struct task {
	uint32_t task_id;
	uint32_t period;
	uint32_t subtask_count;
	uint32_t subtask_start_idx;
	uint32_t exe_time; 
};

struct subtask {
	uint32_t subtask_id;
	uint32_t exe_time;
	uint32_t parent_task_id;
	struct hrtimer timer;
	struct task_struct *thread;
	ktime_t last_release_time;
	uint32_t loop_count;
	uint32_t cumulative_exe_time;
	double util_rate;
	int core;
	double rel_deadline;
	int priority;
};

struct task tasks[] = {
	{.task_id = 0, .period = 6000, .subtask_count = 3, .subtask_start_idx = 0, .exe_time = 5000},
	{.task_id = 1, .period = 4000, .subtask_count = 3, .subtask_start_idx = 3, .exe_time = 1500}
};

struct subtask subtasks[] = {
	{.exe_time = 1000, .core = 0, .priority = 30, .subtask_id = 0, .parent_task_id = 0,	.loop_count = 1e7},
	{.exe_time = 2000, .core = 0, .priority = 31, .subtask_id = 1, .parent_task_id = 0, .loop_count = 2e7},
	{.exe_time = 2000, .core = 0, .priority = 32, .subtask_id = 2, .parent_task_id = 0, .loop_count = 2e7},
	{.exe_time = 500, .core = 1, .priority = 33, .subtask_id = 0, .parent_task_id = 1, .loop_count = 5e6},
	{.exe_time = 500, .core = 1, .priority = 34, .subtask_id = 1, .parent_task_id = 1, .loop_count = 5e6},
	{.exe_time = 500, .core = 1, .priority = 35, .subtask_id = 2, .parent_task_id = 1, .loop_count = 5e6},
};

