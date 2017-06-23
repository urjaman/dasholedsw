#include "main.h"
#include "timer.h"
#include "cron.h"

static struct cron_task *chead = NULL;

static void list_rm_task(struct cron_task *t)
{
	struct cron_task *s = chead;
	struct cron_task *p = NULL;
	while (s) {
		if (s==t) {
			if (!p) {
				chead = s->next;
			} else {
				p->next = s->next;
			}
		}
		p = s;
		s = s->next;
	}
}

static void list_add_task(struct cron_task *t)
{
	/* Assumed: task isnt in the list. */
	struct cron_task * s = chead;
	struct cron_task * p = NULL;
	t->next_invoc += t->ss_freq;
	while (s) {
		if (s->next_invoc > t->next_invoc) {
			break;
		}
		p = s;
		s = s->next;
	}
	if (!p) {
		t->next = chead;
		chead = t;
	} else {
		s = p->next;
		p->next = t;
		t->next = s;
	}
}

void cron_add_task(struct cron_task *t)
{
	list_rm_task(t); /* Check that it isnt in the list. */
	t->next_invoc = timer_get_lin_ss_u16();
	list_add_task(t);
}

void cron_rm_task(struct cron_task *t)
{
	list_rm_task(t);
}

void cron_initialize(void)
{
	if (!chead) return;
	/* Second changed. Change our list timing data. */
	struct cron_task *s = chead;
	while (s) {
		if (s->next_invoc>=SSTC) s->next_invoc -= SSTC;
		else s->next_invoc = 0;
		s = s->next;
	}
}

/* Give timer the next time when to run a task. */
uint16_t cron_next_task(void)
{
	if (!chead) return 0xFFFF;
	return chead->next_invoc;
}

/* Run the next task. */
uint16_t cron_run_tasks(void)
{
	if (!chead) return 0xFFFF;
	struct cron_task *nt = chead;
	nt->taskf();
	chead = nt->next;
	list_add_task(nt);
	return chead->next_invoc;
}
