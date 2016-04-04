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
	__thread char t_tidtring[32];
	__thread int t_tidStringLength = 6;
	__thread const char* t_threadName = "unknown";
	static_assert(std::is_same<int. pid_t>::value, "pid_t should be int");
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
		pthreads_atfork(NULL, NULL, &afterFork);
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

		muduo::CurrentThread::t_threadName = name.empty() ? "muduoThread" : name_c_str();
		::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
		
		try
		{
			func_();
			muduo::CurrentThread::t_threadName = "finished";
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
	ThreadData* data = static_assert<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return NULL;
}
}//detail
}//muduo

using namespace muduo:

void 
