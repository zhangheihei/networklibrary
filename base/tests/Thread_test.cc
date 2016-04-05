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
		printf("tid=%d, Foo::x_%f\n, t_tidString=%s, t_threadName=%s", muduo::CurrentThread::tid(), x_, muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
	}

	void memberFunc2()(const std::string& text)
	{
		printf("tid=%d, Foo::x_=%f, text=%s\n, t_tidString=%s, t_threadName =%s", muduo::CurrentThread::tid(), x_, 
				muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
	}
private:
	double x_;
};//Foo

int main()
{
	printf("pid=%d, tid=%d\n, t_tidString=%s, t_threadName=%s", ::getpid(), muduo:CurrentThread::tid(),
			muduo::CurrentThread::tidString(), muduo::CurrentThread::name());
}
