#include <event2/event.h>

#include <event2/listener.h>

#include <event2/bufferevent.h>

#include <event2/buffer.h>

#include <signal.h>

#include <string.h>

#include <string>

#include <iostream>
#include <zlib.h>
using namespace std;

#define SPORT 5001
struct Status
{
	bool start=false;
	FILE* fp=0;
	z_stream* p=0;
	int recvNum=0;
	int writeNum=0;
	~Status()
	{
		if(fp) fclose(fp);
		fp=0;
		if(p) inflateEnd(p);
		delete p;
		p=0;
	}
	//string filename;
};
static bufferevent_filter_result filter_in(evbuffer* s,evbuffer* d,ev_ssize_t limit,bufferevent_flush_mode mode,void* arg)

{
	Status* status=(Status*)arg;
	//1 接收客户端发送的文件名
	if(!status->start)
	{
		char data[1024]={0};

		int len=evbuffer_remove(s,data,sizeof(data)-1);

		//cout<<"server recv: "<<data<<endl;

		evbuffer_add(d,data,len);
		return BEV_OK;
	}	
	//解压
	evbuffer_iovec v_in[1];
	//读取数据不清零缓冲
	int n=evbuffer_peek(s,-1,NULL,v_in,1);
	if(n<=0)return BEV_NEED_MORE;
	//解压上下文
	z_stream* p=status->p;
	
	//zlib输入数据大小
	p->avail_in=v_in[0].iov_len;
	//zlib输入数据地址
	p->next_in=(Byte*)v_in[0].iov_base;
	
	//申请输出空间大小
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(d,4096,v_out,1);
	
	//zlib输出空间大小
	p->avail_out=v_out[0].iov_len;
	
	//zlib输出空间地址
	p->next_out=(Byte*)v_out[0].iov_base;
	
	//解压数据
	int re=inflate(p,Z_SYNC_FLUSH);
	if(re!=Z_OK)
	{
		cerr<<"deflate failed!\n";
	}
	//解压用了多少数据，从source evbuffer中移除
	//p->avail_in未处理的数据大小
	int nread=v_in[0].iov_len-p->avail_in;
	
	//解压后数据大小，传入des evbuffer
	//p->avail_out剩余空间大小
	int nwrite=v_out[0].iov_len-p->avail_out;
	
	//移除source evbuffer中数据
	evbuffer_drain(s,nread);
	//传入des evbuffer
	v_out[0].iov_len=nwrite;
	evbuffer_commit_space(d,v_out,1);
	cout<<"server nread="<<nread<<" nwrite="<<nwrite<<endl;
	status->recvNum+=nread;
	status->writeNum+=nwrite;
	return BEV_OK;

}

static void read_cb(bufferevent* bev,void* arg)

{
	Status* status=(Status*)arg;
	if(!status->start)
	{
		//001 接收文件名
		char data[1024]={0};
		bufferevent_read(bev,data,sizeof(data)-1);
		//status->filename=data;
		string out="out/";
		out+=data;
		//打开写入文件
		status->fp=fopen(out.c_str(),"wb");
		if(!status->fp)
		{
			cout<<"open file "<<out<<" failed"<<endl;
			return;
		}
		//002 回复ok
		bufferevent_write(bev,"OK",2);
		status->start=true;
		return;
	}
	do
	{
		//写入文件
		char data[1024]={0};
		int len=bufferevent_read(bev,data,sizeof(data));
		if(len>=0)
		{
			fwrite(data,1,len,status->fp);	
			fflush(status->fp);
		}
	}while(evbuffer_get_length(bufferevent_get_input(bev))>0);
}

static void event_cb(bufferevent* bev,short events,void* arg)

{	
	Status* status=(Status*)arg;
	cout<<"server event cb "<<events<<endl;
	if(events&BEV_EVENT_EOF)
	{
		
		cout<<"server event BEV_EVENT_EOF"<<endl;
		cout<<"Server recv "<<status->recvNum;
		cout<<" wrtie "<<status->writeNum<<endl;
		delete status;
		bufferevent_free(bev);
		
	}
}

static void listen_cb(struct evconnlistener* e,evutil_socket_t s,struct sockaddr* a,int socklen,void* arg)

{

	cout<<"listen_cb"<<endl;

	event_base* base=(event_base*)arg;

	//1 创建一个bufferevnet用来通信

	bufferevent* bev=bufferevent_socket_new(base,s,BEV_OPT_CLOSE_ON_FREE);
	Status* status=new Status();
	status->p=new z_stream();
	inflateInit(status->p);
	
	//2 添加输入过滤 并设置输入回调

	bufferevent* bev_filter=bufferevent_filter_new(bev,

		filter_in,//输入过滤函数

		0,//输出过滤函数不处理

		BEV_OPT_CLOSE_ON_FREE,//关闭filter同时关闭bufferevent

		0,//清理回调

		status//传参

	);

	//3 设置回调 读取，事件（处理连接断开）

	bufferevent_setcb(bev_filter,read_cb,0,event_cb,status);

	bufferevent_enable(bev_filter,EV_READ|EV_WRITE);

}

void Server(event_base* base)

{

	cout<<"begin Sever"<<endl;

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

		

}
