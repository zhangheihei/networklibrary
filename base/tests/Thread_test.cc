#include "../Thread.h"
#include "../CurrentThread.h"

#include <string>
#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds)
{
	timespec t = {seconds, 0};
	nanosleep(&t, NULL);
}

void threadFunc()
{
	printf("tid=%d, t_tidString=%s, t_threadName=%s\n", muduo::CurrentThread::tid(), muduo::CurrentThread::tidString(),
			muduo::CurrentThread::name());
}

void threadFunc2(int x)
{
	printf("tid=%d, x=%d. t_tidString=%s, t_threadName=%s\n", muduo::CurrentThread::tid(), x, muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
}

void threadFunc3()
{
	printf("tid=%d, t_tidString=%s, t_threadName=%s\n", muduo::CurrentThread::tid(), muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
	mysleep(1);
	printf("tid=%d, sleep over \n", muduo::CurrentThread::tid());
}

class Foo
{
public:
	explicit Foo(double x)
		: x_(x)
	{
	}

	void memberFunc()
	{
		printf("tid=%d, Foo::x_%f, t_tidString=%s, t_threadName=%s\n", muduo::CurrentThread::tid(), x_, muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
	}

	void memberFunc2(const std::string& text)
	{
		printf("tid=%d, Foo::x_=%f, text=%s, t_tidString=%s, t_threadName =%s\n", muduo::CurrentThread::tid(), x_, text.c_str(), muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
	}
private:
	double x_;
};//Foo

int main()
{
	printf("pid=%d, tid=%d, t_tidString=%s, t_threadName=%s\n", ::getpid(), muduo::CurrentThread::tid(),
			muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
//	printf("pid=%d, tid=%d, t_tidString=%s, t_threadName=%s\n", ::getpid(), muduo::CurrentThread::tid(),
//			t_tidString, t_threadName);
/*
	std::function<void ()>	 testFunc1_ = std::bind(threadFunc);
	muduo::Thread t1(testFunc1_);
	t1.start();
	t1.join();

	std::function<void ()> testFunc2_ = std::bind(threadFunc2, 42);

	muduo::Thread t2(std::move(testFunc2_), "thread for free function with argument(rvalue)");
	t2.start();
	t2.join();

	Foo foo(87.53);
	muduo::Thread t3(std::bind(&Foo::memberFunc, &foo), "thread for member function without argument");
	
	t3.start();
	t3.join();

	muduo::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo), std::string("Shuo CHen")));
	t4.start();
	t4.join();
	*/
	{
	muduo::Thread t5(threadFunc3);
	t5.start();
	}
sleep(2);	
	/*
	{
		muduo::Thread t6(threadFunc3);
		t6.start();
		mysleep(2);
	}
	*/
				
}
