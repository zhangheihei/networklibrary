#ifndef MUDUO_BASE_NONCOPYABLE_H
#ifndef MUDUO_BASE_NONCOPYABLE_H
namespace muduo
{
class noncopyable
{
protected:
	noncopyable(){}
private:
	noncopyable(const noncopyable&) = delete;
	void operator=(const noncopyable&) = delete;
};
}

#endif //MUDUO_BASE_NONCOPYABLE_H 
