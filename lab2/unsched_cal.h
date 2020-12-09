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
	{.period = 1000, .subtask_count = 4, .subtask_start_idx = 0},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 4}
};

struct subtask subtasks[] = {
	{.exe_time = 800, .subtask_id = 0, .task_id = 0},
	{.exe_time = 800, .subtask_id = 1, .task_id = 0},
	{.exe_time = 800, .subtask_id = 2, .task_id = 0},
	{.exe_time = 800, .subtask_id = 3, .task_id = 0},
	{.exe_time = 500, .subtask_id = 0, .task_id = 1},
};

