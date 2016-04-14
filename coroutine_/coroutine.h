#ifndef MY_COROUTINE_H
#define MY_COROUTINE_H
#include <ucontext.h>
#include <vector>

#define DEFAULT_STACK_SIZE (1024*128)

enum CoroutineState{FREE, RUNNABLE, RUNNING, SUSPEND};

struct ScheduleCoroutine;

typedef void (*Fun)(void *arg);

struct Coroutine_t
{
	ucontext_t ctx_; 
	Fun func_;//要执行的函数
	void *arg_; //函数参数
	enum CoroutineState state_; //协程的状态
	char stack_[DEFAULT_STACK_SIZE];//协程的栈
};

typedef std::vector<Coroutine_t> coroutineVector;

struct ScheduleCoroutine
{
	ScheduleCoroutine() : coroutineID_(-1)
	{
	}
	ucontext_t main_; //主函数的上下文信息
	int coroutineID_; //正在执行的协程ID
	coroutineVector coroutines_;
};

static void coroutineBody(ScheduleCoroutine *ps);

//创建协程
int coroutineCreate(ScheduleCoroutine &schedule, Fun func, void* arg);
//挂起调度器ScheduleCoroutine 中当前正在执行的协程，切换到主函数
void coroutineYield(ScheduleCoroutine &schedule);

//恢复调度器ScheduleCoroutine编号为ID的协程
void coroutineResume(ScheduleCoroutine& schedule, int id);

//判断调度器ScheduleCoroutine中所有的协程是否执行完毕，是为1，否为0
int scheduleFinished(const ScheduleCoroutine& schedule);
#endif //MY_COROUTINE_H

