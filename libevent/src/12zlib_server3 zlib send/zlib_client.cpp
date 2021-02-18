#include <event2/event.h>

#include <event2/listener.h>

#include <event2/bufferevent.h>

#include <event2/buffer.h>

#include <signal.h>

#include <string.h>

#include <iostream>
#include <zlib.h>
using namespace std;

#define FILEPATH "001.txt"
struct ClientStatus
{
	FILE* fp=0;
	bool end=false;
	bool startSend=false;
	int readNum=0;
	int sendNum=0;
	z_stream* z_output=0;
	~ClientStatus()
	{
		if(fp) fclose(fp);
		fp=0;
		if(z_output) deflateEnd(z_output);
		delete z_output;
		z_output=0;
	}
};
static bufferevent_filter_result filter_out(evbuffer* s,evbuffer* d,ev_ssize_t limit,bufferevent_flush_mode mode,void* arg)

{
	ClientStatus* sta=(ClientStatus*)arg;
	//压缩文件
	//发送文件消息001 去掉
	if(!sta->startSend)
	{
		char data[1024]={0};
		int len=evbuffer_remove(s,data,sizeof(data));
		evbuffer_add(d,data,len);
		return BEV_OK;
	}
	//开始压缩文件
	//取出buffer中数据的引用
	evbuffer_iovec v_in[1];
	int n=evbuffer_peek(s,-1,0,v_in,1);
	if(n<=0)
	{
		//调用write回调 清理空间
		if(sta->end)
		{
			return BEV_OK;
		}
		//没有数据 不会进入写入回调
		return BEV_NEED_MORE;
	}
	//zlib上下文
	z_stream* p=sta->z_output;
	if(!p)return BEV_ERROR;
	//输入数据大小
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
	//zlib压缩
	int re=deflate(p,Z_SYNC_FLUSH);
	if(re!=Z_OK)
	{
		cerr<<"deflate failed!\n";
	}
	//压缩用了多少数据，从source evbuffer中移除
	//p->avail_in未处理的数据大小
	int nread=v_in[0].iov_len-p->avail_in;
	
	//压缩后数据大小，传入des evbuffer
	//p->avail_out剩余空间大小
	int nwrite=v_out[0].iov_len-p->avail_out;
	
	//移除source evbuffer中数据
	evbuffer_drain(s,nread);
	//传入des evbuffer
	v_out[0].iov_len=nwrite;
	evbuffer_commit_space(d,v_out,1);
	cout<<"client nread="<<nread<<" nwrite="<<nwrite<<endl;
	sta->readNum+=nread;
	sta->sendNum+=nwrite;
	//cout<<"filter_out"<<endl;
	return BEV_OK;

}

static void client_read_cb(bufferevent* bev,void* arg)

{
	ClientStatus* sta=(ClientStatus*)arg;
	
	//002接收服务端发送的ok

	char data[1024]={0};

	int len=bufferevent_read(bev,data,sizeof(data)-1);
	if(strcmp(data,"OK")==0)
	{
		cout<<data<<endl;
		sta->startSend=true;
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
			cout<<"Client read "<<s->readNum<<" send "<<s->sendNum<<endl;
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

		FILE *fp=fopen(FILEPATH,"rb");
		if(!fp)
		{
			cout<<"open file "<<FILEPATH<<" failed"<<endl;
			//return;
		}
		ClientStatus* s=new ClientStatus();
		s->fp=fp;
		//初始化zlib上下文
		s->z_output=new z_stream();
		deflateInit(s->z_output,Z_DEFAULT_COMPRESSION);
				
		//创建输出过滤

		bufferevent* bev_filter=bufferevent_filter_new(be,0,filter_out,
			BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS,0,s);
			
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
