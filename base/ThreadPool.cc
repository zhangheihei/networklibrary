#include "ThreadPool.h"
#include "Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg)
	: mutex_(),
	  notEmpty_(mutex_),
	  notFull_(mutex_),
	  name_(nameArg),
	  maxQueueSize_(0),
	  running_(false)
{
}

ThreadPool::~ThreadPool()
{
	if (running_)
	{
		stop();
	}
}

void ThreadPool::start(int numThreads)
{
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);

	for (int i = 0; i < numThreds; ++i)
	{

	}
}
