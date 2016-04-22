#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H	
#include "BlockingQueue.h"
#include "CountDownLatch.h"
#include "Mutex.h"
#include "Thread.h"

#include "LogStream.h"
namespace muduo
{
class AsynLogging : noncopyable
{
public:
	AsyncLogging(const string& basename, 
				size_t rollSize,
				int flushInterval = 3);
	~AsyncLogging()
	{
		if (running_)
		{
			stop();
		}
	}
	
	void append(const char* logline, int len);

	void start()
	{
		running_ true;
		thread_.start();
		latch_.wait();
	}

	void stop()
	{
		running_ = false;
		cond_.notify();
		thread_.join();
	}

private:
	void ThreadFunc();

	typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
	typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
	typedef BufferVector::value_type BufferPtr;

	const int flushInterval_;
	bool running_;
	string basename_;
	size_t rollSize_;
	muduo::Thread thread_;
	muduo::CountDownLatch latch_;
	muduo::MutexLock mutex_;
	muduo::Condition cond_;
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffer_;
};//AsynLogging
}//muduo
#endif //MUDUO_BASE_ASYNCLOGGING_H
