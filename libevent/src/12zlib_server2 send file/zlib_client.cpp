#include <event2/event.h>

#include <event2/listener.h>

#include <event2/bufferevent.h>

#include <event2/buffer.h>

#include <signal.h>

#include <string.h>

#include <iostream>

using namespace std;

#define FILEPATH "001.txt"
struct ClientStatus
{
	FILE* fp=0;
	bool end=false;
};
static bufferevent_filter_result filter_out(evbuffer* s,evbuffer* d,ev_ssize_t limit,bufferevent_flush_mode mode,void* arg)

{

	cout<<"filter_out"<<endl;
	char data[1024]={0};
	int len=evbuffer_remove(s,data,sizeof(data));
	evbuffer_add(d,data,len);
	return BEV_OK;

}

static void client_read_cb(bufferevent* bev,void* arg)

{

	//002接收服务端发送的ok

	char data[1024]={0};

	int len=bufferevent_read(bev,data,sizeof(data)-1);
	if(strcmp(data,"OK")==0)
	{
		cout<<data<<endl;
		//开始发送文件，触发写入回调
		bufferevent_trigger(bev,EV_WRITE,0);
	}
	else
	{
		bufferevent_free(bev);
	}
	cout<<data<<endl;

	cout<<"client_read_cb"<<endl;

}

static void client_write_cb(bufferevent* bev,void* arg)

{
	ClientStatus* s=(ClientStatus*)arg;
	FILE* fp=s->fp;
	//判断什么时候清理资源
	if(s->end)
	{
		//判断缓冲是否有数据，如果有刷新
		//获取过滤器绑定的buffer
		bufferevent* be=bufferevent_get_underlying(bev);
		//获取输出缓冲及其大小
		evbuffer* evb=bufferevent_get_output(be);
		int len=evbuffer_get_length(evb);
		if(len<=0)
		{
			//立刻清理 如果缓冲有数据不会发送
			bufferevent_free(bev);
			delete s;
			return;
		}
		//刷新缓冲
		bufferevent_flush(bev,EV_WRITE,BEV_FINISHED);
		return;
	}
	
	if(!fp)return;
	cout<<"client_write_cb"<<endl;
	//读取文件
	char data[1024]={0};
	int len=fread(data,1,sizeof(data),fp);
	if(len<=0)
	{
		fclose(fp);
		s->end=true;
		//刷新缓冲
		bufferevent_flush(bev,EV_WRITE,BEV_FINISHED);
		return;
	}
	//发送文件
	bufferevent_write(bev,data,len);
}



static void client_event_cb(bufferevent* be,short events, void* arg)

{

	cout<<"client event_cb"<<events<<endl;

	if(events& BEV_EVENT_CONNECTED)

	{

		cout<<"BEV_EVENT_CONNECTED"<<endl;

		//001发送文件名

		bufferevent_write(be,FILEPATH,strlen(FILEPATH));

		//创建输出过滤

		bufferevent* bev_filter=bufferevent_filter_new(be,0,filter_out,
			BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS,0,0);
			
		FILE *fp=fopen(FILEPATH,"rb");
		if(!fp)
		{
			cout<<"open file "<<FILEPATH<<" failed"<<endl;
			//return;
		}
		ClientStatus* s=new ClientStatus();
		s->fp=fp;
		//设置读取、写入和事件的回调

		bufferevent_setcb(bev_filter,client_read_cb,client_write_cb,client_event_cb,s);

		bufferevent_enable(bev_filter,EV_READ|EV_WRITE);

	}

}

void Client(event_base* base)

{

	cout<<"begin Client"<<endl;

	//连接服务器

	sockaddr_in sin;

	memset(&sin,0,sizeof(sin));

	sin.sin_family=AF_INET;

	sin.sin_port=htons(5001);

	evutil_inet_pton(AF_INET,"127.0.0.1",&sin.sin_addr.s_addr);

	bufferevent* bev=bufferevent_socket_new(base,-1,BEV_OPT_CLOSE_ON_FREE);

	//只绑定事件回调，用来确认连接成功

	bufferevent_enable(bev,EV_READ|EV_WRITE);

	bufferevent_setcb(bev,0,0,client_event_cb,0);

	bufferevent_socket_connect(bev,(sockaddr*)&sin,sizeof(sin));

	

	//接收回复确认OK

}
