#include "AsyncLogging.h"
#include "LogFile.h"
#include "Timestamp.h"

#include <stdio.h>

using namespace muduo;

AsyncLogging::AsyncLogging(const string& basename,
						   size_t rollSize,
						   int flushInterval)
	: flushInterval_(flushInterval),
	  running_(false),
	  basename_(basename),
	  rollSize_(rollSize),
	  thread_(std::bind(&AsyncLogging::threadFunc, this),"Logging"),
	  latch_(1),
	  mutex_(),
	  cond_(mutex_),
	  currentBuffer_(new Buffer),
	  nextBuffer_(new Buffer),
	  buffers_()
{
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16);
}
	  
