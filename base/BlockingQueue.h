#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H
#include "Condition.h"
#include "Mutex.h"

#include <deque>
#include <assert.h>
#include <stdio.h>
namespace muduo
{
template<typename T>
class BlockingQueue : noncopyable
{
public:
	BlockingQueue()
		: mutex_(),
		  notEmpty_(mutex_),
		  queue_()
	{
	}

	void put(const T& x)
	{
		
//		printf("*****************put lvalue\n");
		MutexLockGuard lock(mutex_);
		queue_.push_back(x);
		notEmpty_.notify(); 
	}
	//将notify移出临界区
	void putTwo(const T& x)
	{
		{
//		printf("*****************put lvalue with small\n");
			MutexLockGuard lock(mutex_);
			queue_.push_back(x);
		}
		notEmpty_.notify();
	}

//#ifdef __GXX_EXPERIMENTAL_CXX0X__
	void put(T&& x)
	{
//		printf("*****************put rvalue\n");
		MutexLockGuard lock(mutex_);
		queue_.push_back(std::move(x));
		notEmpty_.notify();
	}

	void putTwo(const T&&x)
	{
		{
//		printf("*****************put rvalue with small\n");
			MutexLockGuard lock(mutex_);
			queue_.push_back(std::move(x));
		}
		notEmpty_.notify();
	}
//#endif //__GXX_EXPERIMENTAL_CXX0X__

	T take()
	{
		MutexLockGuard lock(mutex_);

		while (queue_.empty())
		{
			notEmpty_.wait();
		}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
//		printf("**********************take rvalue\n");
		T front(std::move(queue_.front()));
#else
//		printf("**********************take lvalue\n");
		T front(queue_.front());
#endif
		queue_.pop_front();
		return front;
	}

	size_t size() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.size();
	}
private:
	mutable MutexLock mutex_;
	Condition notEmpty_;
	std::deque<T> queue_;
};
}//muduo


#endif //MUDUO_BASE_BLOCKINGQUEUE_H
