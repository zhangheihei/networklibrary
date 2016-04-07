#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H


#include "CurrentThread.h"
#include "noncopyable.h"
#include <assert.h>
#include <pthread.h>

#ifdef CHECK_PTHREAD_RETURN_VALUE
#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail(int errnum, 
								 const char *file, 
								 unsigned int line, 
								 const char *function)
	__THROW __attribute__ ((__noreturn__));
__END_DECLS
#endif  //NDEBUG

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret); \
					   if (__builtin_expect(errnum != 0, 0))  \
						__assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})
#else // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret) ({__typeof__ (ret) errnum = (ret);  assert(errnum == 0); (void) errnum;})

#endif //CHECK_PTHREAD_RETURN_VALUE

namespace muduo
{
class MutexLock : noncopyable
{
public:
	MutexLock() : holder_(0)
	{
		MCHECK(pthread_mutex_init(&mutex_, NULL));
	}

	~MutexLock()
	{
		assert(holder_ == 0);
		MCHECK(pthread_mutex_destroy(&mutex_));
	}

	bool isLockedByThisThread() const
	{
		return holder_ == CurrentThread::tid();
	}

	void assertLocked() const
	{
		assert(isLockedByThisThread());
	}

	void lock()
	{
		MCHECK(pthread_mutex_lock(&mutex_));
		assignHolder();
	}

	void unlock()
	{
		unassignHolder();
		MCHECK(pthread_mutex_unlock(&mutex_));
	}

	pthread_mutex_t* getPthreadMutex()
	{
		return &mutex_;
	}

private:
	friend class Condition;
	class UnassignGuard : noncopyable
	{
	 public:
		UnassignGuard(MutexLock& owner) : owner_(owner)
		{
			owner_.unassignHolder();
		}

		~UnassignGuard()
		{
			owner_.assignHolder();
		}
	 private:
		MutexLock& owner_;
	};

	void unassignHolder()
	{
		holder_ = 0;
	}

	void assignHolder()
	{
		holder_ = CurrentThread::tid();
	}

	pthread_mutex_t mutex_;
	pid_t holder_;
}; // Mutex

class MutexLockGuard : noncopyable
{
public:
	explicit MutexLockGuard(MutexLock& mutex)
		: mutex_(mutex)
	{
		mutex_.lock();
	}

	~MutexLockGuard()
	{
		mutex_.unlock();
	}
private:
	MutexLock& mutex_;
}; // MutexLockGuard

#define MutexLockGuard(x) error "Missing guard object name"


}//muduo
#endif  //MUDUO_BASE_MUTEX_H
