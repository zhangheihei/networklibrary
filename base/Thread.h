#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H
#include "Atomic.h"
#include "Types.h"

#include <functional>
#include <memory>
#include <pthread.h>
namespace muduo
{
class Thread : noncopyable
{
public:
	typedef std::function<void ()> ThreadFunc;
	
	explicit Thread(const ThreadFunc&, const string& name = string());
#ifdef __GXX_EXPERIMENTAL_CXX0X__
	explicit Thread(ThreadFunc&&, const string& name = string());
#endif

	~Thread();

	void start();
	int join(); // return pthread_join()

	bool started() const { return started_; }

	pid_t tid() const {return *tid_; }
	const string& name() const {return name_; }

	static int numCreated() { return numCreated_.get(); }
private:
	void setDefaultName();

	bool started_;
	bool joined_;
	pthread_t pthreadId_;
	std::shared_ptr<pid_t> tid_;
	ThreadFunc func_;
	string name_;

	static AtomicInt32 numCreated_;
};//Thread
}//muduo


#endif //MUDUO_BASE_THREAD_H
