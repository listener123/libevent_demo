#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <signal.h>
#include <string.h>
#include <iostream>
using namespace std;
#define SPORT 5000
bufferevent_filter_result filter_in(evbuffer* s,evbuffer* d,ev_ssize_t limit,bufferevent_flush_mode mode,void* arg)
{
	cout<<"filter_in\n";

	char data[1024]={0};
	//读取并清理原数据
	int len=evbuffer_remove(s,data,sizeof(data)-1);
	//所有字母转成大写
	for(int i=0;i<len;i++)
	{
		data[i]=toupper(data[i]);
	}
	evbuffer_add(d,data,len);
	return BEV_OK;
}
bufferevent_filter_result filter_out(evbuffer* s,evbuffer* d,ev_ssize_t limit,bufferevent_flush_mode mode,void* arg)
{
	cout<<"filter_out\n";
	char data[1024]={0};
	//读取并清理原数据
	int len=evbuffer_remove(s,data,sizeof(data)-1);
	string str="";
	str+="===================\n";
	str+=data;
	str+="===================\n";
	evbuffer_add(d,str.c_str(),str.size());
	return BEV_OK;
}
void read_cb(bufferevent* bev,void* arg)
{
	cout<<"read_cb\n";
	char data[1024]={0};
	int len=bufferevent_read(bev,data,sizeof(data));
	cout<<data<<endl;
	//回复客户消息，经过输出过滤
	bufferevent_write(bev,data,len);
}
void write_cb(bufferevent* bev,void* arg)
{
	cout<<"write_cb\n";
}
void event_cb(bufferevent* bev,short events,void* arg)
{
	cout<<"event_cb\n";
}
void listen_cb(struct evconnlistener* e,evutil_socket_t s,struct sockaddr* a,int socklen,void* arg)
{
	cout<<"listen_cb"<<endl;
	event_base* base=(event_base*)arg;
	//创建bufferevent 
	bufferevent* bev=bufferevent_socket_new(base,s,BEV_OPT_CLOSE_ON_FREE);
	//绑定bufferevent filter
	bufferevent* bev_filter=bufferevent_filter_new
	(
		bev,
		filter_in,				//输入过滤函数
		filter_out,				//输出过滤函数
		BEV_OPT_CLOSE_ON_FREE,	//关闭filter是同时关闭bufferevent
		0,						//清理的回调函数
		0						//传递给回调的参数
	);
	//设置bufferevent的回调
	bufferevent_setcb(bev_filter,read_cb,write_cb,event_cb,NULL);//回调函数的参数
	bufferevent_enable(bev_filter,EV_READ|EV_WRITE);
}
int main()
{
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	cout<<"test server\n";
	event_base* base=event_base_new();
	if(base)
	{
		cout<<"event_base_new success"<<endl;
	}
	//监听事件
	//socket bind listen绑定事件
	sockaddr_in sin;
	memset(&sin,0,sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_port=htons(SPORT);
	
	evconnlistener* ev = evconnlistener_new_bind(base,	//libevent上下文
		listen_cb,					//接收到连接的回调函数
		base,						//回调函数获取的参数arg
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,	//地址重用，listen关闭同时关闭socket
		10,						//连接队列大小，对应listen函数
		(sockaddr*)&sin,				//绑定的地址和端口
		sizeof(sin)
		);
	//事件分发处理
	if(base)
		event_base_dispatch(base);
	if(ev)
		evconnlistener_free(ev);
	if(base)
		event_base_free(base);
	return 0;
	
}
