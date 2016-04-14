#include "coroutine.h"



int coroutineCreate(ScheduleCoroutine& schedule, Fun func, void* arg)
{
	int id = 0;
	int coroutineNum = schedule.coroutines_.size();

	for (id = 0; id < coroutineNum; id++)
	{
		if (schedule.coroutines_[id].state_ == FREE)
		{
			break;
		}
	}

	if (id == coroutineNum)
	{
		Coroutine_t coroutine;
		schedule.coroutines_.push_back(coroutine);
	}

	Coroutine_t* ct = &(schedule.coroutines_[id]);

	ct->state_ = RUNNABLE;
	ct->func_ = func;
	ct->arg_ = arg;

	return id;
}

void coroutineBody(ScheduleCoroutine *ps)
{
	int id = ps->coroutineID_;
	if (id != -1)
	{
		Coroutine_t* ct = &(ps->coroutines_[id]);

		ct->func_(ct->arg_);

		ct->state_ = FREE;

		ps->coroutineID_ = -1;
	}
}

void coroutineYield(ScheduleCoroutine &schedule)
{
	if (schedule.coroutineID_ != -1)
	{
		Coroutine_t* ct = &(schedule.coroutines_[schedule.coroutineID_]);
		ct->state_ = SUSPEND;
		schedule.coroutineID_ = -1;

		swapcontext(&(ct->ctx_), &(schedule.main_));
	}
}

void coroutineResume(ScheduleCoroutine& schedule, int id)
{
	if (id < 0 || id > schedule.coroutines_.size())
	{
		return;
	}

	Coroutine_t* ct = &(schedule.coroutines_[id]);

	switch (ct->state_)
	{
		case RUNNABLE :
			getcontext(&(ct->ctx_));

			ct->ctx_.uc_stack.ss_sp = ct->stack_;
			ct->ctx_.uc_stack.ss_size = DEFAULT_STACK_SIZE;
			ct->ctx_.uc_stack.ss_flags = 0;
			ct->ctx_.uc_link = &(schedule.main_);
			ct->state_ = RUNNING;

			schedule.coroutineID_ = id;

			makecontext(&(ct->ctx_), (void(*)(void))(coroutineBody), 1, &schedule);
		case SUSPEND :
			swapcontext(&(schedule.main_), &(ct->ctx_));

			break;
		default: ;

	}
}


int scheduleFinished(const ScheduleCoroutine& schedule)
{
	if (schedule.coroutineID_ != -1)
	{
		return 0;
	}
	else
	{
		for (int i = 0; i < schedule.coroutines_.size(); ++i)
		{
			if (schedule.coroutines_[i].state_ != FREE)
			{
				return 0;
			}
		}
	}

	return 1;
}

