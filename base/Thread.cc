#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include "Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{
namespace CurrentThread
{
	__thread int t_cachedTid = 0;
	__thread char t_tidString[32];
	__thread int t_tidStringLength = 6;
	__thread const char* t_threadName = "unknown";
	static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");
}//CurrentThread

namespace detail
{
pid_t gettid()
{
	return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
	muduo::CurrentThread::t_cachedTid = 0;
	muduo::CurrentThread::t_threadName = "main";
	CurrentThread::tid();
}

class ThreadNameInitializer
{
public:
	ThreadNameInitializer()
	{
		muduo::CurrentThread::t_threadName = "main";
		CurrentThread::tid();
		pthread_atfork(NULL, NULL, &afterFork);
	}
};//ThreadNameInitializer

ThreadNameInitializer init;

struct ThreadData
{
	typedef muduo::Thread::ThreadFunc ThreadFunc;
	ThreadFunc func_;
	string name_;
	std::weak_ptr<pid_t> wkTid_;

	ThreadData(const ThreadFunc& func,
			   const string& name,
			   const std::shared_ptr<pid_t>& tid)
		: func_(func),
		  name_(name),
		  wkTid_(tid)
	{ }

	void runInThread()
	{
		pid_t tid = muduo::CurrentThread::tid();
		
		std::shared_ptr<pid_t> ptid = wkTid_.lock();

		if (ptid)
		{
			*ptid = tid;
			ptid.reset();
		}

		muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
		::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
		
		try
		{
			func_();
			muduo::CurrentThread::t_threadName = "finished";
		//	printf("tid=%d, t_threadName=%s\n", muduo::CurrentThread::t_cachedTid, muduo::CurrentThread::t_threadName);
		}
		catch (const Exception& ex)
		{
			muduo::CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
			abort();
		}
		catch (const std::exception& ex)
		{
			muduo::CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
			fprintf(stderr, "reason: %s\n", ex.what());
			abort();
		}
		catch (...)
		{
			muduo::CurrentThread::t_threadName = "crashed";
			fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
			throw; // rethrow
		}

	}//runInThread
};//ThreadData

void* startThread(void* obj)
{
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return NULL;
}
}//detail
}//muduo

using namespace muduo;

void CurrentThread::cacheTid()
{
	if (t_cachedTid == 0)
	{
		t_cachedTid = detail::gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof t_tidString
				, "%5d ", t_cachedTid);
	}
}

bool CurrentThread::isMainThread()
{
	return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
	struct timespec ts = {0, 0};
	ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
	ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond);
	::nanosleep(&ts, NULL);
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const string& n)
	: started_(false),
	  joined_(false),
	  pthreadId_(0),
	  tid_(new pid_t(0)),
	  func_(func),
	  name_(n)
{

//	printf("tid=%d use construct\n", muduo::CurrentThread::t_cachedTid);
	setDefaultName();
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
Thread::Thread(ThreadFunc&& func, const string& n)
	: started_(false),
	  joined_(false),
	  pthreadId_(0),
	  tid_(new pid_t(0)),
	  func_(std::move(func)),
	  name_(n)
{
	//printf("tid=%d use move construct\n", muduo::CurrentThread::t_cachedTid);
	setDefaultName();
}
#endif

Thread::~Thread()
{
	if (started_ && !joined_)
	{
		pthread_detach(pthreadId_);
	}
//	printf("destructor\n");
}

void Thread::setDefaultName()
{
	int num = numCreated_.incrementAndGet();
	if (name_.empty())
	{
		char buf[32];
		snprintf(buf, sizeof buf, "Thread%d", num);
		name_ = buf;
	}
}

void Thread::start()
{
	assert(!started_);
	started_ = true;
	detail::ThreadData* data = new detail::ThreadData(func_, name_, tid_);
	if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
	{
		started_ = false;
		delete data;
		LOG_SYSFATAL << "Failed in pthread_create";
	}
}

int Thread::join()
{
	assert(started_);
	assert(!joined_);
	joined_ = true;
	return pthread_join(pthreadId_, NULL);
}
