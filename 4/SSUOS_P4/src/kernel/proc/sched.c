#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>

extern struct list plist;
extern struct list rlist;
extern struct list runq[RQ_NQS];

extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;
struct process *latest;

bool more_prio(const struct list_elem *a, const struct list_elem *b,void *aux);
int scheduling; 					// interrupt.c

struct process* get_next_proc(void) 
{
	bool found = false;
	struct process *next = NULL;
	struct list_elem *elem;
	int i;

	/* 
	   You shoud modify this function...
	   Browse the 'runq' array 
	 */

	/*
	   for(elem = list_begin(&rlist); elem != list_end(&rlist); elem = list_next(elem))
	   {
	   struct process *p = list_entry(elem, struct process, elem_stat);

	   if(p->state == PROC_RUN)
	   return p;
	   }
	 */
	for (i = 0; i < RQ_NQS; i++) {
		if(!list_empty(&runq[i])) {
			for (elem = list_begin(&runq[i]); elem != list_end(&runq[i]); elem = list_next(elem)) {
				next = list_entry(elem, struct process, elem_stat);

				if (next->state == PROC_RUN) {
					return next;
				}
			}
		}
	}

	return NULL;
}

void schedule(void)
{
	struct process *cur;
	struct process *next;

	/* You shoud modify this function.... */
	struct list_elem *elem;
	int check = 0;

	if (cur_process->pid != 0) {
		scheduling = 1; 
		cur = cur_process;
		cur_process = idle_process;
		cur_process->time_slice = 0;
		scheduling = 0;

		intr_disable();
		switch_process(cur, idle_process);
		intr_enable();

		return;
	}

	proc_wake();

	for (elem = list_begin(&plist); elem != list_end(&plist); elem = list_next(elem)) {
		if ((cur = list_entry(elem, struct process, elem_all))->state == PROC_RUN && cur->pid != 0) {
			if (check != 0) {
				printk(", ");
			}
			else {
				check++;
			}
			printk("#= %d p= %d c= %d u= %d", cur->pid, cur->priority, cur->time_slice, cur->time_used);
		}

	}
	if (check != 0) {
		printk("\n");
	}

	if (cur_process->time_slice == 60) {
		cur_process->time_slice = 0;
	}

	if ((next = get_next_proc()) == NULL) {
		printk("get_next_proc() error\n");
		exit(1);
	}
	cur = cur_process;
	cur_process = next;

	if (next->pid != 0) {
		cur_process->time_slice = 0;
		printk("Selected # = %d\n", next->pid);
	}

	intr_disable();
	switch_process(cur, next);
	intr_enable();
}
