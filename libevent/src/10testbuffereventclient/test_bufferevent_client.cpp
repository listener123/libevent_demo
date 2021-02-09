#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <signal.h>
#include <iostream>
#include <string.h>
#include <string>
using namespace std;

static string recvstr="";
static int recvCount=0;
static int sendCount=0;
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
		//读取缓冲中数据
		char data[1024]={0};
		//读取输入缓冲数据
		int len=bufferevent_read(be,data,sizeof(data)-1);
		if(len>0)
		{
			recvCount+=len;
			recvstr+=data;
		}
	
		cout<<"BEV_EVENT_READING BEV_EVENT_TIMEOUT"<<endl;
		//bufferevent_enable(be,EV_READ);
		bufferevent_free(be);
		cout<<recvstr<<endl;
		cout<<"recvCount="<<recvCount<<" "<<"sendCount="<<sendCount<<endl;
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
	//cout<<"["<<data<<"]"<<endl;
	cout<<data<<flush;
	
	if(len<=0)return;
	recvstr+=data;
	recvCount+=len;
	//if(strstr(data,"quit")!=NULL)
	//{
	//	cout<<"quit";
		//退出并关闭socket 由于BEV_OPT_CLOSE_ON_FREE
	//	bufferevent_free(be);
	//}
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
	timeval t1={0,500000};
	bufferevent_set_timeouts(bev,&t1,0);
	//设置回调函数
	bufferevent_setcb(bev,read_cb,write_cb,event_cb,base);
	
}
void client_write_cb(bufferevent* be,void* arg)
{
	cout<<"[Client_W]"<<flush;
	FILE* fp=(FILE*)arg;
	char data[1024]={0};
	int len=fread(data,1,sizeof(data)-1,fp);
	if(len<=0)
	{
		//读到结尾或文件出错
		fclose(fp);
		//立刻清理，可能会造成缓冲数据没有发送结束
		//bufferevent_free(be);
		bufferevent_disable(be,EV_WRITE);
		return;
	}
	sendCount+=len;
	//写入buffer
	bufferevent_write(be,data,len);
}
//错误、超时、连接断开会进入
void client_event_cb(bufferevent* be,short events,void* arg)
{
	cout<<"[Client_E]"<<flush;
	//读取超时事件发生后，数据读取停止
	if(events&BEV_EVENT_TIMEOUT && events&BEV_EVENT_READING)
	{
		cout<<"BEV_EVENT_READING BEV_EVENT_TIMEOUT"<<endl;
		//bufferevent_enable(be,EV_READ);
		bufferevent_free(be);
		return;
	}
	else if(events & BEV_EVENT_ERROR)
	{
		bufferevent_free(be);
		return;
	}
	//服务端的关闭事件
	if(events & BEV_EVENT_EOF)
	{
		cout<<"BEV_EVENT_EOF"<<endl;
		bufferevent_free(be);
	}
	if(events& BEV_EVENT_CONNECTED)
	{
		cout<<"BEV_EVENT_CONNECTED"<<endl;
		//触发write
		bufferevent_trigger(be,EV_WRITE,0);
	}

}
void client_read_cb(bufferevent* be,void* arg)
{
	cout<<"[Client_R]"<<flush;
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
	
	
	{
		//调用客户端代码
		//-1内部创建socket
		bufferevent* bev=bufferevent_socket_new(base,-1,BEV_OPT_CLOSE_ON_FREE);
		sockaddr_in sin;
		memset(&sin,0,sizeof(sin));
		sin.sin_family=AF_INET;
		sin.sin_port=htons(5001);
		evutil_inet_pton(AF_INET,"127.0.0.1",&sin.sin_addr.s_addr);
		FILE *fp=fopen("test_bufferevent_client.cpp","rb");
		//设置回调函数
		bufferevent_setcb(bev,client_read_cb,client_write_cb,client_event_cb,fp);
		bufferevent_enable(bev,EV_READ|EV_WRITE);
		int re=bufferevent_socket_connect(bev,(sockaddr*)&sin,sizeof(sin));
		if(re==0)
		{
			cout<<"connect"<<endl;
		}
	}
	//进入事件主循环
	event_base_dispatch(base);
	evconnlistener_free(ev);
	event_base_free(base);
	
	return 0;
	
}
