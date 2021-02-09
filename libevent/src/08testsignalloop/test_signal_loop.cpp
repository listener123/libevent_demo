#include <event2/event.h>
#include <signal.h>
#include <iostream>
using namespace std;
bool isexit=false;
//sock文件描述符，which事件类型，arg传递参数
static void Ctrl_C(int sock,short which,void* arg)
{
	cout<<"Ctrl_C"<<endl;
	event_base* base=(event_base*)arg;
	//执行完当前处理事件函数就退出
	//event_base_loopbreak(base);
	//运行完所有活动事件再退出，事件循环没有运行时也要等运行一次退出
	timeval t={3,0}; //至少运行3秒后退出
	event_base_loopexit(base,&t);
}
static void Kill(int sock,short which,void* arg)
{
	cout<<"Kill"<<endl;
	event* ev=(event*)arg;
	//如果处于非待决
	if(!evsignal_pending(ev,NULL))
	{
		event_del(ev);
		event_add(ev,NULL);
	}
}
int main(int argc,char* argv[])
{
	event_base* base=event_base_new();
	//添加信号ctrl +C,处于no pending
	//evsignal_new隐藏状态EV_SIGNAL|EV_PERSIST
	event* csig=evsignal_new(base,SIGINT,Ctrl_C,base);
	if(!csig)
	{
		cerr<<"SIGNINT evsignal_new failed\n";
		return -1;
	}
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	cout<<"test server\n";
	//添加事件到pending
	if(event_add(csig,0)!=0)
	{
		cerr<<"SIGNINT event_add failed\n";
		return -1;
	}
	
	//添加kill信号
	//非持久事件只进入一次，event_self_cbarg()传递当前的event
	event* ksig=event_new(base,SIGTERM,EV_SIGNAL,Kill,event_self_cbarg());
	
	if(event_add(ksig,0)!=0)
	{
		cerr<<"SIGTERN event_add failed\n";
		return -1;
	}
	
	//进入事件主循环
	//event_base_dispatch(base);
	//EVLOOP_ONCE 等待一个事件运行，直到没有活动事件就退出返回0
	//event_base_loop(base,EVLOOP_ONCE);
	//EVLOOP_NONBLOCK 有活动事件就处理，没有就返回0
	/*while(!isexit)
	{
		event_base_loop(base,EVLOOP_NONBLOCK);
	}
	*/
	//EVLOOP_NO_EXIT_ON_EMPTY 没有注册事件也不返回 用于事件后期多线程添加
	event_base_loop(base,EVLOOP_NO_EXIT_ON_EMPTY);
	event_free(csig);
	event_base_free(base);
	
	return 0;
	
}
//ctril z
//pkill XXX
//ps
//pkill -9 XXX
