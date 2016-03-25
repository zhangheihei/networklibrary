#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H
#include "LogStream.h"
#include "Timestamp.h"
namespace muduo
{
class TimeZone;

class Logger
{
public:
	enum LogLevel
	{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		NUM_LOG_LEVELS,
	}; //LogLevel
// compile time calculation of basename of source file
	class SourceFile
	{
	 public:
		 template<int N>
		 inline SourceFile(const char (&arr)[N])
		 	: data_(arr),
			  size_(N-1)
		 {
			 const char* slash = strrchr(data_, '/');
			 if (slash)
			 {
				 data_ = slash + 1;
				 size_ -= static_cast<int>(data_ - arr);
			 }
		 }

		 explicit SourceFile(const char* filename)
			 : data_(filename)
		{
			const char* slash = strrchr(filename, '/');
			if (slash)
			{
				data_ = slash + 1;
			}
			size_ = static_cast<int>(strlen(data_));
		}

		 const char* data_;
		 int size_;

			   
	};//SourceFile

	Logger(SourceFile file, int line);
	Logger(SourceFile file, int line, LogLevel level);
	Logger(SourceFile file, int line, LogLevel level, const char* func);
	Logger(SourceFile file, int line, LogLevel);
}; //Logger
}//muduo


#endif //MUDUO_BASE_LOGGING_H
