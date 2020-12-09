import subprocess
import sys
import os

mode = sys.argv[1]

sys_log = subprocess.run(["dmesg"], stdout=subprocess.PIPE, text=True)
f = sys_log.stdout.strip().split('\n')
out = """
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

struct task tasks[] = {"""

for line in f:
    if line.find("TASK_FIN") != -1 and line.find("SUBTASK_FIN") == -1:
        out += '\n\t' + line[line.find("{"):] + ','

out += "\n};\n"
out += "struct subtask subtasks[] = {"

for line in f:
    if line.find("SUBTASK_FIN") != -1:
        out += '\n\t' + line[line.find("{"):] + ','

out += "\n};\n"

with open(mode + ".h", 'w+') as f:
    f.write(out)

