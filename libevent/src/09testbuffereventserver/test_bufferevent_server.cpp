#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <signal.h>
#include <iostream>
#include <string.h>
using namespace std;
void write_cb(bufferevent* be,void* arg)
{
	cout<<"[W]"<<flush;
}
//错误、超时、连接断开会进入
void event_cb(bufferevent* be,short events,void* arg)
{
	cout<<"[E]"<<flush;
	//读取超时事件发生后，数据读取停止
	if(events&BEV_EVENT_TIMEOUT && events&BEV_EVENT_READING)
	{
		cout<<"BEV_EVENT_READING BEV_EVENT_TIMEOUT"<<endl;
		//bufferevent_enable(be,EV_READ);
		bufferevent_free(be);
	}
	else if(events & BEV_EVENT_ERROR)
	{
		bufferevent_free(be);
	}
	else
	{
		cout<<"OTHERS"<<endl;
	}
}
void read_cb(bufferevent* be,void* arg)
{
	cout<<"[R]"<<flush;
	char data[1024]={0};
	//读取输入缓冲数据
	int len=bufferevent_read(be,data,sizeof(data)-1);
	cout<<"["<<data<<"]"<<endl;
	if(len<=0)return;
	if(strstr(data,"quit")!=NULL)
	{
		cout<<"quit";
		//退出并关闭socket 由于BEV_OPT_CLOSE_ON_FREE
		bufferevent_free(be);
	}
	//发送数据 写入到输出缓冲
	bufferevent_write(be,"OK",3);
	
}
void listen_cb(evconnlistener* ev,evutil_socket_t s,sockaddr* sin,int slen,void * arg)
{
	cout<<"listen_cb\n";
	event_base* base=(event_base*)arg;
	//创建bufferevent上下文BEV_OPT_CLOSE_ON_FREE清理bufferevent时关闭socket
	bufferevent* bev=bufferevent_socket_new(base,s,BEV_OPT_CLOSE_ON_FREE);
	//添加监控事件
	bufferevent_enable(bev,EV_READ|EV_WRITE);
	//设置水位
	//读取水位
	bufferevent_setwatermark(bev,EV_READ,
		5,	//低水位
		10	//高水位0是无限制，默认0
	);
	bufferevent_setwatermark(bev,EV_WRITE,
		5,	//低水位 缓冲数据低于5 写入回调被调用
		0	//高水位0是无限制，默认0
	);
	//超时时间的设置
	timeval t1={3,0};
	bufferevent_set_timeouts(bev,&t1,0);
	//设置回调函数
	bufferevent_setcb(bev,read_cb,write_cb,event_cb,base);
	
}
int main(int argc,char* argv[])
{
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	event_base* base=event_base_new();
	//创建网络服务器
	//设定监听的端口和地址
	sockaddr_in sin;
	memset(&sin,0,sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_port=htons(5001);
	evconnlistener* ev=evconnlistener_new_bind(base,
	listen_cb,	//回调函数
	base,		//回调函数的参数arg
	LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
	10,			//listen back
	(sockaddr*)&sin,
	sizeof(sin)
	);
	//进入事件主循环
	event_base_dispatch(base);
	evconnlistener_free(ev);
	event_base_free(base);
	
	return 0;
	
}
//ctril z
//pkill XXX
//ps
//pkill -9 XXX
