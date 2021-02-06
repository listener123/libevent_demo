#include <event2/event.h>
#include <signal.h>
#include <iostream>
using namespace std;
//sock文件描述符，which事件类型，arg传递参数
static void Ctrl_C(int sock,short which,void* arg)
{
	cout<<"Ctrl_C"<<endl;
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
	event_base_dispatch(base);
	event_free(csig);
	event_base_free(base);
	
	return 0;
	
}
//ctril z
//pkill XXX
//ps
//pkill -9 XXX
