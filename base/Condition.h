#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H
#include "Mutex.h"
#include <pthread.h>

namespace muduo
{
class Condition : noncopyable
{
public:
	explicit Condition(MutexLock& mutex) : mutex_(mutex)
	{
		MCHECK(pthread_cond_init(&pcond_, NULL));
	}

	~Condition()
	{
		MCHECK(pthread_cond_destroy(&pcond_));
	}

	void wait()
	{
		MutexLock::UnassignGuard ug(mutex_);
		MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
	}
	bool waitForSeconds(int seconds);

	void notify()
	{
		MCHECK(pthread_cond_signal(&pcond_));
	}

	void notifyAll()
	{
		MCHECK(pthread_cond_broadcast(&pcond_));
	}
private:
	MutexLock& mutex_;
	pthread_cond_t pcond_;
};//Condition


}//muduo
#endif //MUDUO_BASE_CONDITION_H
