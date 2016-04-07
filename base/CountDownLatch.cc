#include "CountDownLatch.h"

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
	:mutex_(),
	 conditon_(mutex_),
	 count_(count)
{
}

void CountDownLatch::wait()
{
	MutexLockGuard lock(mutex_);
	while (count_ > 0)
	{
		condition_.wait();
	}
}

void CountDownLatch::countDown()
{
	MutexLockGuard lock(mutex_);
	--count_;

	if (count_ == 0)
	{
		conditon_.notifyAll();
	}
}

int CountDownLatch::getCount() const
{
	MutexLockGuard lock(mutex_);
	return count_;
}
