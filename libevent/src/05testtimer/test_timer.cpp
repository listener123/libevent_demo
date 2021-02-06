#include <event2/event.h>
#include <signal.h>
#include <iostream>
#include <thread>
#include <chrono>         // std::chrono::seconds
using namespace std;
static timeval t1={1,0};
void timer3(int sock,short which, void* arg)
{
	cout<<"[timer3]"<<flush;
}

void timer2(int sock,short which, void* arg)
{
	cout<<"[timer2]"<<flush;
	this_thread::sleep_for(chrono::seconds(3));
}
void timer1(int sock,short which, void* arg)
{
	cout<<"[timer1]"<<flush;
	event *ev=(event*)arg;
	//no pending
	if(!evtimer_pending(ev,&t1))
	{
		evtimer_del(ev);
		evtimer_add(ev,&t1);
	}
}
int main(int argc,char* argv[])
{
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	event_base* base=event_base_new();
	//定时器
	cout<<"test timer"<<endl;
	//event_new
	//非持久事件定时器
	event* ev1=evtimer_new(base,timer1,event_self_cbarg());
	if(!ev1)
	{
		cout<<"evtimer_new timer1 failured\n";
		return -1;
	}
	evtimer_add(ev1,&t1);//插入性能O(logn)
	
	static timeval t2;
	t2.tv_sec=1;
	t2.tv_usec=200000;//微秒
	//EV_PERSIST持久事件
	event* ev2=event_new(base,-1,EV_PERSIST,timer2,0);
	event_add(ev2,&t2);
	
	//超时优化性能优化，默认event用二叉堆存储（完全二叉树）插入删除O(logn)
	//优化到双向队列 插入删除O(1)
	static timeval tv_in={3,0};
	const timeval* t3;
	t3=event_base_init_common_timeout(base,&tv_in);
	event* ev3=event_new(base,-1,EV_PERSIST,timer3,0);
	event_add(ev3,t3);//插入性能O(1)
	//进入事件主循环
	event_base_dispatch(base);
	event_base_free(base);
	
	return 0;
	
}

