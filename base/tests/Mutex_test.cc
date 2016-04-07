#include "../CountDownLatch.h"
#include "../Mutex.h"
#include "../Thread.h"
#include "../Timestamp.h"

#include <vector>
#include <stdio.h>

using namespace muduo;
using namespace std;

MutexLock g_mutex;
vector<int> g_vec;
const int kCount = 10*1000*1000;

void threadFunc()
{
	for (int i = 0; i < kCount; ++i)
	{
		MutexLockGuard lock(g_mutex);
		g_vec.push_back(i);
	}
}

int foo() __attribute__ ((noinline));

int g_count = 0;
int foo()
{
	MutexLockGuard lock(g_mutex);
	if (!g_mutex.isLockedByThisThread())
	{
		printf("FAIL\n");
		return -1;
	}
	++g_count;
	return 0;
}


int main()
{

}
