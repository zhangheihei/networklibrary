#include "../BlockingQueue.h"
#include "../CountDownLatch.h"
#include "../Thread.h"
#include "../Timestamp.h"
#include <memory>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

using std::placeholders::_1;

class Test
{
public:
	Test(int numThreads)
		: latch_(numThreads)
	{
		for (int i = 0; i < numThreads; ++i)
		{
			char name[32];
			snprintf(name, sizeof name, "work thread %d", i);
			threads_.emplace_back(new  muduo::Thread(std::bind(&Test::threadFunc, this), muduo::string(name)));
		}

		for (auto& thr : threads_)
		{
			thr->start();
		}
	}

	void runWithoutRValueAndBigCriticalSection(int times)
	{
		printf("waiting for count down latch\n");
		latch_.wait();
		printf("all threads started\n");
		for (int i = 0; i < times; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf,"hello %d", i);
			std::string str1(buf);
			queue_.put(str1);
			printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
		}

	}//runWithoutRValueAndBigCriticalSection
	
	void runWithoutRvalueAndSmallCriticalSection(int times)
	{
		
		printf("waiting for count down latch\n");
		latch_.wait();
		printf("all threads started\n");
		for (int i = 0; i < times; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf,"hello %d", i);
			std::string str1(buf);
			queue_.putTwo(str1);
			printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
		}
	}//runWithoutRvalueAndSmallCriticalSection
	void  runWithRValueAndBigCriticalSection(int times)
	{
		printf("waiting for count down latch\n");
		latch_.wait();
		printf("all threads started\n");
		for (int i = 0; i < times; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf,"hello %d", i);
			std::string str1(buf);
			queue_.put(std::move(str1));
			printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
		}

	} //

	void runWithRValueAndSmallCriticalSection(int times)
	{

		printf("waiting for count down latch\n");
		latch_.wait();
		printf("all threads started\n");
		for (int i = 0; i < times; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf,"hello %d", i);
			std::string str1(buf);
			queue_.putTwo(std::move(str1));
			printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
		}
	}

	void joinAll()
	{
		for (size_t i = 0; i < threads_.size(); ++i)
		{
			std::string str("stop");
			queue_.put(std::move(str));
		}

		for (auto& thr : threads_)
		{
			thr->join();
		}
	}//joinAll

private:
	void threadFunc()
	{
		printf("tid=%d, %s started\n",
				muduo::CurrentThread::tid(),
				muduo::CurrentThread::name());

		latch_.countDown();
		bool running = true;
		while (running)
		{
			std::string d(queue_.take());
			printf("tid = %d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());
			running = (d != "stop");
		}

		printf("tid=%d, %s stopped\n", muduo::CurrentThread::tid(), muduo::CurrentThread::name());
	}

	muduo::BlockingQueue<std::string> queue_;
	muduo::CountDownLatch latch_;
	std::vector<std::unique_ptr<muduo::Thread>> threads_;
};//Test


int main()
{
	printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());
//普通构造函数加大临界区	
	muduo::Timestamp start1(muduo::Timestamp::now()); 
	Test t(8);
//	t.run(100000);
	t.runWithoutRValueAndBigCriticalSection(100000);	
	t.joinAll();
	double end1 = timeDifference(muduo::Timestamp::now(), start1);


//普通构造函数加小临界区
	muduo::Timestamp start2(muduo::Timestamp::now());
	Test t2(8);
	t2.runWithoutRvalueAndSmallCriticalSection(100000);
	t2.joinAll();
	double end2 = timeDifference(muduo::Timestamp::now(), start2);

//移动构造函数加大临界区

	muduo::Timestamp start3(muduo::Timestamp::now());
	Test t3(8);
	t3.runWithRValueAndBigCriticalSection(100000);
	t3.joinAll();
	double end3 = timeDifference(muduo::Timestamp::now(), start3);

//移动构造函数加小临界区
 
	muduo::Timestamp start4(muduo::Timestamp::now());
	Test t4(8);
	t4.runWithRValueAndSmallCriticalSection(100000);
	t4.joinAll();
	double end4 = timeDifference(muduo::Timestamp::now(), start4);


	printf("thread without rvalue with Big Critical Section %f\n", end1);
	printf("thread without rvalue with small Critical Section %f\n", end2);
	printf("thread with rvalue with Big Critical Section %f\n", end3);
	printf("thread with rvalue with small Critical Section %f\n", end4);
	printf("number of created threads %d\n", muduo::Thread::numCreated());
}
