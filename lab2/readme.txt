# WE have a rich text version of this report. It is named Lab2-Report.pdf

----------MEMBERS-------
	Haodong Huang
	Bingxin Liu

----------MODULE DESIGN----------

Data Structure

In our replaceable header file, we defined two structures: task and subtask.

The task structure including:

    the period field denoting the period the task should be released,

    the subtask_count field, which on behalf of the total number of subtasks within each task,

    the task_start_idx field recording the first subtask's index in the array of subtasks,

    and the exe_time field, which is the sum of the execution time of all its subtasks.

The subtask structure is consisted of:

    the task_id field, which records which task the subtask is belonging to,

    the subtask_id field, the index for subtask,

    the exe_time field, the time used by subtask in each release,

    the timer field, which is used to wake up its successor,

    the thread field, a pointer to the subtask thread's task_struct,

    the last_realease_time field, which is recording when the subtask was released the last time,

    the loop_count field, which is calculated via "calibrate" mode and specify how many times the subtask should loop,

    the cumulative_exe_time field, recording the sum of each its precessors' execution time

    the core field, calculated in "calibrate" mod and represent which core the subtask should run on,

    and the priority field specified in "calibrate" mod, and denote the priority value of the subtask's thread.

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


Calibration Mode

In calibration mode, there are basically two jobs to do: automatically calculating the parameters for tasks and subtasks before creating the calibration thread in the init function, and in each calibration thread calibrating the loop count of each subtask on the same core using binary search.

There are totally 4 parameters to calculate before calibrating loop count: subtasks’ cumulative execution time, core, priority and tasks’ execution time. Implementation details are as follows:

1) Subtasks’ cumulative execution time: For each task, iterate through its subtasks topologically based on the subtask_start_idx and subtask_count fields stored in its struct. During the iteration, compute the value on the fly: the summation of previous subtask’s cumulative execution time (0 if it is the first) and the current subtask’s execution time.
2) Tasks’ execution time: Equivalent to the last subtask’s cumulative execution time. Could be calculated during the calculation of subtasks’ cumulative execution time.

3) Subtasks’ core: Firstly allocate a new array of length named sub_ptr storing all subtasks’ pointers. Sort the array in descending order of utilization. Note that float division is not recommended in kernel so the utilization comparison is achieved by converting to equivalent product comparison. In detail, the compare function comp_util takes two subtask struct pointers A, B as parameters, obtains their parent task structs by the task_id field and compares utilization by comparing equivalent integer multiplication: (A->exe_time) * (parent(B)->period) and (B->exe_time) * (parent(A)->period). After sorting the array, iterate through the array and assign a proper core sequentially based on the aggregate utilization status of each core. Use array agg_time[CORE_NUM] to store the total execution time within a hyper period. Then during the iteration for each subtask, first mark its core as CORE_NUM which represents an invalid case(valid core IDs are 0, 1, …, CORE_NUM - 1). Then sequentially iterate i-th core and check whether agg_time[i] will exceed the hyper period after assigning the subtask to this core. If not, assign it to this core and break. If finally the core ID is still CORE_NUM, then this is an unschedulable case so just assign the subtask to core 0.

4) Subtasks’ priority: Again sort the sub_ptr array in ascending order of the union {core ID, relative deadline}. To deal with the float division issue of relative deadline, again an equivalent product comparison could be used: (parent(A)->period) * (A->cumulative_exe_time) * (parent(B)->execution_time) and (parent(B)->period) * (B->cumulative_exe_time) * (parent(A)->execution_time). After sorting the array, iterate the array sequentially and assign the priority value based on the following strategy: If it is the first subtask of the array or the subtask’s core ID is different from the previous one’s core ID, assign 50 to its priority; Otherwise assign previous one’s priority minus 1 to its priority. This ensures that priority values are assigned in descending order on each core.

Then after calculating all these parameters, in the init function create a calibration thread for each core, bind the thread to that core, set the scheduling policy to SCHED_FIFO with a priority of 20 and then wake up the thread such that the thread suspends itself when it calls the schedule() function. Finally set a timer which expires after 100ms and upon expiration it wakes up all calibration threads simultaneously. To ensure that i-th thread could access all subtasks’ structs on i-th core, pass the address of the index in array sub_ptr at which the element points to the first subtask struct assigned to i-th core. This method works since during calculating priority, the sub_ptr array is sorted such that all structs with the same core are contiguous in the array.

In each calibration thread, iterate all subtask structs in the sub_ptr array beginning with the address passed to the thread, until the core ID changes. To calibrate each subtask’s loop count, use binary search with a lower bound of 0 and upper bound of 1e8 (around 10s). We choose binary search since it is of logarithm complexity, which utilizes the monotonicity elegantly. It is also more stable and accurate than directly calculating the linear coefficient. Finally output the subtask information to system log including the task id, subtask id, loop count, core and priority.


Run Mode

In the run mode, we firstly set the last release time of every subtask as zero. Then create a thread for every subtask via kthread_create function, and assign the pointer to task_struct to the field of the thread within every subtask structure. Then, a specific CPU core will be bound to this subtask thread. Also, the scheduler is set for every subtask, and then each subtask thread is wake up. In this case, the CPU core number and priority in the scheduler are calculated in the calibrate mode. Then we initialize a timer, assign the timer function, which, in the run mode, will wake up all tasks' first subtask, and then start the timer.

In the thread function of run mode (i.e every subtask's thread function), we first initialize the timer of each subtask, and then set thread interruptible and hung up the thread waiting for a wake-up signal in a while loop, which will determine whether the loop should stop via kthread_shoud_stop function. We then update the last release time in the thread function and record the release time by printk function. Immediately after update the last release time, we release the subtask function by calling the subtask_func function, which takes a pointer to subtask structure as its argument and runs specific times(set in the subtask structure in calibrate mode) of ktime_get() instruction. After finishing the subtask function, we first check if the current subtask is missing its relative deadline, which is simply the task's period * current task's cumulative execution time / total execution time. If the subtask missing its relative deadline, we report this case via printk. Because our task is released periodically, to keep this feature, we set and start a timer, which will wake up the subtask's thread periodically if the subtask is the first subtask of a task. Then, if the subtask that running is not the last one, we simply release the successive subtask by wake the successive subtask's thread, if the time right now exceeds the sum of the task period and it's successor's last release time. Otherwise, we finish the subtask early, so we then schedule the successive subtask's timer to wake up one task period after its successors' last release time. Right now, every task is released periodically as their period, and every subtask is released one by one in order.

When we hope the kernel module exit, we just simply stop all of the subtasks' threads via the kthread_stop() function, cancel each subtask's timer, and finally cancel the timer that is used to wake up all subtasks' thread at the beginning.


----------TESTING AND EVALUATION----------

CASE A:

For the case that everything is correctly configured and scheduled, we have made several tests, and finally, find that 90 percent utilization of each core is the best performance that we can do. In this case, we have five tasks. The first three tasks have a 1000ms period and 510ms execution time, and each task only has exactly one subtask. The fourth task is released in a 1000ms period and has three subtasks and a 750ms execution time. Therefore each subtask has a 250ms execution time. The last task also has a 1000ms period and three subtasks but only have a 420ms execution time. As a result, each of its subtasks has a 140ms execution time. Finally, our utilization in each core is around 90 percent. The detailed, field-calculated data structure is shown below:

struct task tasks[] = {
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 0, .exe_time = 510},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 1, .exe_time = 510},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 2, .exe_time = 510},
	{.period = 1000, .subtask_count = 3, .subtask_start_idx = 3, .exe_time = 750},
	{.period = 1000, .subtask_count = 3, .subtask_start_idx = 6, .exe_time = 420},
};
struct subtask subtasks[] = {
	{.task_id = 0, .subtask_id = 0, .exe_time = 510, .loop_count = 5052186, .core = 0, .priority = 48, .cumulative_exe_time = 510},
	{.task_id = 1, .subtask_id = 0, .exe_time = 510, .loop_count = 5052917, .core = 2, .priority = 48, .cumulative_exe_time = 510},
	{.task_id = 2, .subtask_id = 0, .exe_time = 510, .loop_count = 5033108, .core = 1, .priority = 48, .cumulative_exe_time = 510},
	{.task_id = 3, .subtask_id = 0, .exe_time = 250, .loop_count = 2710750, .core = 2, .priority = 50, .cumulative_exe_time = 250},
	{.task_id = 3, .subtask_id = 1, .exe_time = 250, .loop_count = 2700451, .core = 1, .priority = 50, .cumulative_exe_time = 500},
	{.task_id = 3, .subtask_id = 2, .exe_time = 250, .loop_count = 2539852, .core = 0, .priority = 49, .cumulative_exe_time = 750},
	{.task_id = 4, .subtask_id = 0, .exe_time = 140, .loop_count = 1581588, .core = 0, .priority = 50, .cumulative_exe_time = 140},
	{.task_id = 4, .subtask_id = 1, .exe_time = 140, .loop_count = 1465037, .core = 2, .priority = 49, .cumulative_exe_time = 280},
	{.task_id = 4, .subtask_id = 2, .exe_time = 140, .loop_count = 1465228, .core = 1, .priority = 49, .cumulative_exe_time = 420},
};
In the kernel shark, we grep all of the context-switch to check everything is good. The screenshot is shown in the file named sched_u90. As we can see, all of the subtasks are scheduled correctly and finished properly before the deadline.
Note that we were not assigning any subtask on CPU 3. This is because we reserved one CPU core for other working processes like trace-cmd. Especially, this part of working will occupy a lot of CPU utilization, which is proved by the graph lined after CPU 3. Therefore, we just leave the CPU 3 to the OS and other working processes and make another three CPU cores ideal, so that we can calculate the best performance(i.e utilization) in a relative ideal situation. The detailed log is included in the file named sched.log.

CASE B:

In this case, we test the situation that all subtasks are configured correctly but unschedulable. The detailed and arguments-calculated data structure is shown below. As we can see, there are still 5 tasks, and the first three tasks have a 1000ms period, 510ms execution time, and one subtask each. As a result, each task's subtask has about 51percent utilization for each CPU core. The fourth task has a 2000ms period, 1800ms execution time, and three 600ms subtasks so that each subtask has 30 percent utilization for its bound CPU core. The fifth task has a 4000ms period, 2800ms execution time, and three subtasks. But in this time, the first subtask of the fifth task will have 2000ms execution time, and the other two subtasks will have 400ms execution time each. Therefore, these three subtasks will make use of 50 percent, 10 percent, and 10 percent utilization of a single CPU. Consequently, the first three tasks will be bind to each CPU core 0 to 2. Then the first subtask of task3 will be bound to core0. Now CPU 0 has 81 percent utilization. Similarly, the second and third subtask of the task3 will be bound to core1 and core2, respectively. Then, each CPU core will get 81 percent utilization. However, the first subtask will need 50 percent of utilization, it is, therefore, unschedulable in this case. We just bind this subtask to core0 as the requirement. Then, the utilization of core0 is over 100 percent, so we assign the other two subtasks to core1 and core2 since they only need 10 percent of utilization and we have left around 19 percent of utilization in core1 and core2.

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
In the kernel shark, we grep all of the context-switch to check what happens. The screenshot is shown in the file named unsched.
As we can see, the CPU core0 is fully used, and in the console, deadline missing is reported:
   [19223.964110] MISSED DDL: task 3, subtask 0
Because the total utilization of each core is over 90 percent, we also observe that some other tasks' deadlines were also missed. The detailed log is included in the file named unsched.log.

CASE C:

In this case, we have the same configuration as case A. However, we manually reduce about 1 million times of loop for each of the subtasks of the task4. Therefore, the actual running time of each subtask of task 4 should around to 40ms, rather than 140ms. The detailed and arguments-calculated data structure is shown below.
struct task tasks[] = {
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 0, .exe_time = 510},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 1, .exe_time = 510},
	{.period = 1000, .subtask_count = 1, .subtask_start_idx = 2, .exe_time = 510},
	{.period = 1000, .subtask_count = 3, .subtask_start_idx = 3, .exe_time = 750},
	{.period = 1000, .subtask_count = 3, .subtask_start_idx = 6, .exe_time = 420},
};
struct subtask subtasks[] = {
	{.task_id = 0, .subtask_id = 0, .exe_time = 510, .loop_count = 4052186, .core = 0, .priority = 48, .cumulative_exe_time = 510},
	{.task_id = 1, .subtask_id = 0, .exe_time = 510, .loop_count = 4052917, .core = 2, .priority = 48, .cumulative_exe_time = 510},
	{.task_id = 2, .subtask_id = 0, .exe_time = 510, .loop_count = 4033108, .core = 1, .priority = 48, .cumulative_exe_time = 510},
	{.task_id = 3, .subtask_id = 0, .exe_time = 250, .loop_count = 2710750, .core = 2, .priority = 50, .cumulative_exe_time = 250},
	{.task_id = 3, .subtask_id = 1, .exe_time = 250, .loop_count = 2700451, .core = 1, .priority = 50, .cumulative_exe_time = 500},
	{.task_id = 3, .subtask_id = 2, .exe_time = 250, .loop_count = 2539852, .core = 0, .priority = 49, .cumulative_exe_time = 750},
	{.task_id = 4, .subtask_id = 0, .exe_time = 140, .loop_count = 581588, .core = 0, .priority = 50, .cumulative_exe_time = 140},
	{.task_id = 4, .subtask_id = 1, .exe_time = 140, .loop_count = 465037, .core = 2, .priority = 49, .cumulative_exe_time = 280},
	{.task_id = 4, .subtask_id = 2, .exe_time = 140, .loop_count = 465228, .core = 1, .priority = 49, .cumulative_exe_time = 420},
};
In the kernel shark, we grep all of the context-switch to check what happens. The screenshot is shown in the file named missched. In this case, we can find that the running time of each subtask of task4 is significantly shorter than before. However, there is no event that happens, such as, the deadline is missed, the task released too early, and over-run and under-run of some subtasks' executions. The detailed log is included in the file named missched.log.


----------BUILD INSTRUCTIONS----------

The kernel module uses a CASE macro to indicate which header file should be included. If CASE is 0, “sched.h” is included, which corresponds to a schedulable case. If CASE is 1, “missched.h” is included, which corresponds to an unschedulable case. If CASE is 2, “missched.h” is included, which corresponds to a mis-configured schedulable case.

Cross compiling is used to compile the kernel module. Upload the .c file and corresponding .h file and the Makefile to the same folder on a CEC server machine. Then simply use command “make” to compile and then download the .ko file to the Raspberry Pi to execute. If calibration mode is used, use command “sudo insmod {module_name}.ko”. Otherwise use command “sudo insmod {module_name}.ko mode=“run” ”. A userspace tool gen.py could be used to automatically generate the header file for run mode after calculating parameters in calibration mode. This tool is implemented by Python3 and parses the system log to get the subtask information, so do not clear the system log right after running in calibration mode. To use the tool, firstly ensure that the calibration module is removed since the subtask information is printed in the exit function of the module. Then simply use command “python3 gen.py {output_header_name}”. For example, command “python3 gen.py sched” would generate file “sched.h” under the same folder. Then recompile the module with the new header to execute in run mode.

----------DEVELOPMENT EFFORT----------

Estimate of the amount of time:

    Haodong Huang:  36 hours
    Bingxin Liu:    36 hours


----------EXTRA CREDIT----------
We implement the first extension which automatically computes the parameters in calibration mode. Also an additional Python3 userspace tool is implemented to parse the system log and generate the header file used in run mode. The parameter calculation details are already introduced in Module Design section. The userspace tool firstly uses Subprocess module in Python3 to obtain the system log content by command “dmesg” and then generate the header based on system log lines beginning with special marks. The usage of the tool is already introduced in Build Instruction section.
