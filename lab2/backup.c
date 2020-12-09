#define CASE 2

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
#include <linux/sort.h>

#if CASE == 0
	#include "sched.h"
#elif CASE == 1
	#include "unsched.h"
#else
	#include "missched.h"
#endif

#define CORE_NUM 3

static char* mode = "calibrate";
module_param(mode, charp, 0644);

static struct task_struct *cal_thread[CORE_NUM];
static struct hrtimer cal_timer;
static uint32_t task_tot = sizeof(tasks) / sizeof(struct task);
static uint32_t subtask_tot = sizeof(subtasks) / sizeof(struct subtask);
static struct subtask **sub_ptr = NULL;
static struct subtask **start_addr[CORE_NUM] = {NULL};

static void subtask_func(struct subtask *t) {
	uint32_t i;
	for(i = 0; i < t->loop_count; i++) {
		ktime_get();
	}
}

static int comp_util(const void *lhs, const void *rhs) {
	struct subtask *l = *(struct subtask * const *)lhs, *r = *(struct subtask * const *)rhs;
	struct task *lpa = &tasks[l->task_id], *rpa = &tasks[r->task_id];
	if(1LL * l->exe_time * rpa->period > 1LL * r->exe_time * lpa->period) return -1;
	if(1LL * l->exe_time * rpa->period < 1LL * r->exe_time * lpa->period) return 1;
	return 0;
}

static int comp_rel_ddl(const void *lhs, const void *rhs) {
	struct subtask *l = *(struct subtask * const *)lhs, *r = *(struct subtask * const *)rhs;
	struct task *lpa = &tasks[l->task_id], *rpa = &tasks[r->task_id];
	if(l->core < r->core) return -1;
	if(l->core > r->core) return 1;
	if(1LL * lpa->period * l->cumulative_exe_time * rpa->exe_time < 1LL * rpa->period * r->cumulative_exe_time * lpa->exe_time) return -1;
	if(1LL * lpa->period * l->cumulative_exe_time * rpa->exe_time > 1LL * rpa->period * r->cumulative_exe_time * lpa->exe_time) return 1;
	return 0;
}


static int calibrate(void *data) {
	struct subtask **cur = (struct subtask **)data;
	uint32_t core = (*cur)->core, min_cnt, max_cnt, mid;
	ktime_t start_t, end_t; 
	struct sched_param sp;

	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	if(core == CORE_NUM) return 0;

	while((*cur)->core == core) {
		sp.sched_priority = (*cur)->priority;
		sched_setscheduler(current, SCHED_FIFO, &sp);
		min_cnt = 0, max_cnt = 1e8;
		while(min_cnt < max_cnt) {
			mid = (min_cnt + max_cnt) >> 1;
			(*cur)->loop_count = mid;
			start_t = ktime_get();
			subtask_func(*cur);
			end_t = ktime_get();
			if(end_t - start_t >= (*cur)->exe_time * NSEC_PER_MSEC) max_cnt = mid;
			else min_cnt = mid + 1;
		}
		(*cur)->loop_count = max_cnt;
		printk(KERN_INFO "SUBTASK: {.task_id = %u, .subtask_id = %u, .loop_count = %u, .core = %u, .priority = %u}\n", 
									(*cur)->task_id, (*cur)->subtask_id, (*cur)->loop_count, (*cur)->core, (*cur)->priority);
		cur++;
	}
    return 0;
}

static enum hrtimer_restart cal_timer_func(struct hrtimer *t) {
	uint32_t i;
	if(strcmp(mode, "calibrate") == 0) {
		for(i = 0; i < CORE_NUM; i++) wake_up_process(cal_thread[i]);
	}
	else {
		for(i = 0; i < task_tot; i++) {
			wake_up_process(subtasks[tasks[i].subtask_start_idx].thread);
		}
	}
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
	struct task *parent = &tasks[cur->task_id];
	struct subtask *successor;
	ktime_t ts, ddl, period = ms_to_ktime(parent->period);

	hrtimer_init(&cur->timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	cur->timer.function = run_timer_func;
	while(!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		cur->last_release_time = ktime_get();
		printk(KERN_INFO "RELEASE: task %u, subtask %u, ts %lld\n", cur->task_id, cur->subtask_id, ktime_to_ms(cur->last_release_time));
		subtask_func(cur);
		
		ts = ktime_get();
		ddl = ms_to_ktime(parent->period * cur->cumulative_exe_time / parent->exe_time);
		ddl = ktime_add(ddl, subtasks[parent->subtask_start_idx].last_release_time);
		if(ktime_before(ddl, ts)) {	
			printk(KERN_INFO "MISSED DDL: task %u, subtask %u\n", cur->task_id, cur->subtask_id);
		}
		
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
	uint32_t i, j, cur_idx, agg_time[CORE_NUM] = {0}, hyper_period = 0;
	struct sched_param sp = {.sched_priority = 20};
	struct task *parent;
	int invalid = 0;

    printk(KERN_INFO "Loaded module in %s mode\n", mode);


	if(strcmp(mode, "calibrate") == 0) {
		for(i = 0; i < task_tot; i++) {
			if(tasks[i].period > hyper_period) hyper_period = tasks[i].period;
			for(j = 0; j < tasks[i].subtask_count; j++) {
				cur_idx = tasks[i].subtask_start_idx + j;
				subtasks[cur_idx].cumulative_exe_time = ((j == 0) ? 0 : subtasks[cur_idx - 1].cumulative_exe_time) + subtasks[cur_idx].exe_time;
				if(j == tasks[i].subtask_count - 1) tasks[i].exe_time = subtasks[cur_idx].cumulative_exe_time;
			}
		}
		
		sub_ptr = kmalloc((subtask_tot + 1) * sizeof(struct subtask *), GFP_KERNEL);
		for(i = 0; i < subtask_tot; i++) sub_ptr[i] = &subtasks[i];
		sub_ptr[subtask_tot] = kmalloc(sizeof(struct subtask), GFP_KERNEL);
		sub_ptr[subtask_tot]->core = CORE_NUM;

		sort(sub_ptr, subtask_tot, sizeof(struct subtask *), &comp_util, NULL);
		for(i = 0; i < subtask_tot; i++) {
			sub_ptr[i]->core = CORE_NUM;
			parent = &tasks[sub_ptr[i]->task_id];
			for(j = 0; j < CORE_NUM; j++) {
				if(agg_time[j] + sub_ptr[i]->exe_time * hyper_period / parent->period <= hyper_period) {
					agg_time[j] += sub_ptr[i]->exe_time * hyper_period / parent->period; 
					sub_ptr[i]->core = j;
					break;
				}
			}
			if(sub_ptr[i]->core == CORE_NUM) {
				invalid = 1;
				sub_ptr[i]->core = 0;
				agg_time[0] = hyper_period;
			}
		}
		sort(sub_ptr, subtask_tot, sizeof(struct subtask *), &comp_rel_ddl, NULL);
		for(i = 0; i < subtask_tot; i++) {
			if(i == 0 || sub_ptr[i]->core != sub_ptr[i - 1]->core) {
				sub_ptr[i]->priority = 50;
				start_addr[sub_ptr[i]->core] = &sub_ptr[i];
			}
			else sub_ptr[i]->priority = sub_ptr[i - 1]->priority - 1;
		}
		for(i = 0; i < CORE_NUM; i++) {
			if(start_addr[i] == NULL) start_addr[i] = &sub_ptr[subtask_tot];
		}
		
		for(i = 0; i < CORE_NUM; i++) {
			cal_thread[i] = kthread_create(calibrate, start_addr[i], "cal_%d", i);
			kthread_bind(cal_thread[i], i);
			sched_setscheduler(cal_thread[i], SCHED_FIFO, &sp);
			wake_up_process(cal_thread[i]);
		}
		hrtimer_init(&cal_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cal_timer.function = cal_timer_func;
		hrtimer_start(&cal_timer, ktime_set(0, 1e8), HRTIMER_MODE_REL);

		if(invalid) {
			printk(KERN_INFO "Unschedulable case\n");
		}
		
	}
	else {
		for(i = 0; i < subtask_tot; i++) {
			subtasks[i].last_release_time = ktime_set(0, 0);
			subtasks[i].thread = kthread_create(run, &subtasks[i], "run_task%d_sub%d", subtasks[i].task_id, subtasks[i].subtask_id);
			kthread_bind(subtasks[i].thread, subtasks[i].core);
			sp.sched_priority = subtasks[i].priority;
			sched_setscheduler(subtasks[i].thread, SCHED_FIFO, &sp);
			wake_up_process(subtasks[i].thread);
		}
		hrtimer_init(&cal_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cal_timer.function = cal_timer_func;
		hrtimer_start(&cal_timer, ktime_set(0, 1e8), HRTIMER_MODE_REL);
	}
    return 0;
}

static void E2E_exit(void) {
	uint32_t i;
	printk(KERN_INFO "Unloaded module\n");
	if(strcmp(mode, "calibrate") == 0) {
		for(i = 0; i < task_tot; i++) {
			printk(KERN_INFO "TASK_FIN: {.period = %u, .subtask_count = %u, .subtask_start_idx = %u, .exe_time = %u}\n", 
							tasks[i].period, tasks[i].subtask_count, tasks[i].subtask_start_idx, tasks[i].exe_time);
		}
		for(i = 0; i < subtask_tot; i++) {
			printk(KERN_INFO "SUBTASK_FIN: {.task_id = %u, .subtask_id = %u, .exe_time = %u, .loop_count = %u, .core = %u, .priority = %u, .cumulative_exe_time = %u}\n",
		        subtasks[i].task_id, 
				subtasks[i].subtask_id, subtasks[i].exe_time,  
				subtasks[i].loop_count, subtasks[i].core, 
				subtasks[i].priority, subtasks[i].cumulative_exe_time);
		}
		kfree(sub_ptr[subtask_tot]);
		kfree(sub_ptr);
	}
	else {
		for(i = 0; i < subtask_tot; i++) {
			kthread_stop(subtasks[i].thread);
			hrtimer_cancel(&subtasks[i].timer);
		}
	}
	hrtimer_cancel(&cal_timer);
}

module_init(E2E_init);
module_exit(E2E_exit);

MODULE_LICENSE ("GPL");
