#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <uapi/linux/sched/types.h>
#include "tasks.h"

static char* mode = "calibrate";
module_param(mode, charp, 0644);

static struct task_struct *cal_thread[4];
static struct hrtimer cal_timer;

static void subtask_func(struct subtask *t) {
	int i;
	for(i = 0; i < t->loop_count; i++) {
		ktime_get();
	}
}


static int calibrate(void *data) {
	int core = * (int*) data;
	int i, subtask_tot = sizeof(subtasks) / sizeof(struct subtask);
	int min_cnt, max_cnt, mid;
	ktime_t start_t, end_t; 
	struct sched_param sp;

	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	
	for(i = 0; i < subtask_tot; i++) {
		if(subtasks[i].core != core) continue;
		sp.sched_priority = subtasks[i].priority;
		sched_setscheduler(current, SCHED_FIFO, &sp);
		min_cnt = 0, max_cnt = 1e8;
		while(min_cnt < max_cnt) {
			mid = (min_cnt + max_cnt) >> 1;
			subtasks[i].loop_count = mid;
			start_t = ktime_get();
			subtask_func(&subtasks[i]);
			end_t = ktime_get();
			if(end_t - start_t >= subtasks[i].exe_time * NSEC_PER_MSEC) max_cnt = mid;
			else min_cnt = mid + 1;
		}
		subtasks[i].loop_count = max_cnt;
		printk(KERN_INFO "CALIBRATE: task %u, subtask %u, loop count %u\n", subtasks[i].parent_task_id, subtasks[i].subtask_id, subtasks[i].loop_count);
	}
    return 0;
}

static enum hrtimer_restart cal_timer_func(struct hrtimer *t) {
	int i;
	for(i = 0; i < 4; i++) wake_up_process(cal_thread[i]);
	return HRTIMER_NORESTART;
}

static struct subtask* subtask_lookup(struct hrtimer *t) {
	return container_of(t, struct subtask, timer);
}

static enum hrtimer_restart run_timer_func(struct hrtimer *t) {
	struct subtask *sub = subtask_lookup(t);
	wake_up_process(sub->thread);
	return HRTIMER_NORESTART;
}

static int run(void *data) {
	struct subtask *cur = (struct subtask *)data;
	struct task *parent = &tasks[cur->parent_task_id];
	struct subtask *successor;
	ktime_t ts, period = ms_to_ktime(parent->period);
		
	hrtimer_init(&cur->timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	cur->timer.function = run_timer_func;
	while(!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		cur->last_release_time = ktime_get();
		printk(KERN_INFO "RELEASE: task %u, subtask %u, ts %lld\n", parent->task_id, cur->subtask_id, ktime_to_ms(cur->last_release_time));
		subtask_func(cur);
		if(cur->subtask_id == 0) {
			hrtimer_start(&cur->timer, ktime_add(cur->last_release_time, period), HRTIMER_MODE_ABS);
		}
		if(cur->subtask_id < parent->subtask_count - 1) {
			successor = &subtasks[parent->subtask_start_idx + cur->subtask_id + 1];
			ts = ktime_get();
			if(ktime_compare(ktime_set(0, 0), successor->last_release_time) == 0 || ktime_compare(ts, ktime_add(period, successor->last_release_time)) >= 0) {
				wake_up_process(successor->thread);
			}
			else {
				hrtimer_start(&successor->timer, ktime_add(period, successor->last_release_time), HRTIMER_MODE_ABS);
			}
		}
	}
	return 0;
}

static int E2E_init(void) {
	int i, tot;
	static int core_id[4] = {0, 1, 2, 3};
	struct sched_param sp = {.sched_priority = 20};

    printk(KERN_INFO "Loaded module in mod %s\n", mode);
	if(strcmp(mode, "calibrate") == 0) {
		for(i = 0; i < 4; i++) {
			cal_thread[i] = kthread_create(calibrate, &core_id[i], "cal_%d", i);
			kthread_bind(cal_thread[i], i);
			sched_setscheduler(cal_thread[i], SCHED_FIFO, &sp);
			wake_up_process(cal_thread[i]);
		}
		hrtimer_init(&cal_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cal_timer.function = cal_timer_func;
		hrtimer_start(&cal_timer, ktime_set(0, 1e8), HRTIMER_MODE_REL);
	}
	else {
		tot = sizeof(subtasks) / sizeof(struct subtask);
		for(i = 0; i < tot; i++) {
			subtasks[i].last_release_time = ktime_set(0, 0);
		}
		for(i = 0; i < tot; i++) {
			subtasks[i].thread = kthread_create(run, &subtasks[i], "run_task%d_sub%d", tasks[subtasks[i].parent_task_id].task_id, subtasks[i].subtask_id);
			kthread_bind(subtasks[i].thread, subtasks[i].core);
			sp.sched_priority = subtasks[i].priority;
			sched_setscheduler(subtasks[i].thread, SCHED_FIFO, &sp);
			wake_up_process(subtasks[i].thread);
		}
		tot = sizeof(tasks) / sizeof(struct task);
		for(i = 0; i < tot; i++) {
			wake_up_process(subtasks[tasks[i].subtask_start_idx].thread);
		}
	}
    return 0;
}

static void E2E_exit(void) {
	int tot, i;
	printk(KERN_INFO "Unloaded module\n");
	if(strcmp(mode, "calibrate") == 0) {
		hrtimer_cancel(&cal_timer);
	}
	else {
		tot = sizeof(subtasks) / sizeof(struct subtask);
		for(i = 0; i < tot; i++) {
			kthread_stop(subtasks[i].thread);
			hrtimer_cancel(&subtasks[i].timer);
		}
	}
}

module_init(E2E_init);
module_exit(E2E_exit);

MODULE_LICENSE ("GPL");
