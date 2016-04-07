#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H
#include "Condition.h"
#include "Mutex.h"
namespace muduo
{
class CountDownLatch : noncopyable
{
public:
	explicit countDownLatch(int count);

	void wait();

	void countDown();

	int getCount() const;
private:
		mutable MutexLock mutex_;
		Condition condition_;
		int count_;
};//countDownLatch
}//muduo

#endif //MUDUO_BASE_COUNTDOWNLATCH_H
