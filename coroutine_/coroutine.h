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
	Fun func_;//Ҫִ�еĺ���
	void *arg_; //��������
	enum CoroutineState state_; //Э�̵�״̬
	char stack_[DEFAULT_STACK_SIZE];//Э�̵�ջ
};

typedef std::vector<Coroutine_t> coroutineVector;

struct ScheduleCoroutine
{
	ScheduleCoroutine() : coroutineID_(-1)
	{
	}
	ucontext_t main_; //����������������Ϣ
	int coroutineID_; //����ִ�е�Э��ID
	coroutineVector coroutines_;
};

static void coroutineBody(ScheduleCoroutine *ps);

//����Э��
int coroutineCreate(ScheduleCoroutine &schedule, Fun func, void* arg);
//���������ScheduleCoroutine �е�ǰ����ִ�е�Э�̣��л���������
void coroutineYield(ScheduleCoroutine &schedule);

//�ָ�������ScheduleCoroutine���ΪID��Э��
void coroutineResume(ScheduleCoroutine& schedule, int id);

//�жϵ�����ScheduleCoroutine�����е�Э���Ƿ�ִ����ϣ���Ϊ1����Ϊ0
int scheduleFinished(const ScheduleCoroutine& schedule);
#endif //MY_COROUTINE_H

