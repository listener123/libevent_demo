#include <event2/event.h>
#include <signal.h>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>         // std::chrono::seconds
using namespace std;
void read_file(evutil_socket_t fd,short event,void* arg)
{
	char buf[1024]={0};
	int len=read(fd,buf,sizeof(buf)-1);
	if(len>0)
	{
		cout<<buf<<endl;
	}
	else
	{
		cout<<"."<<flush;
		this_thread::sleep_for(std::chrono::seconds(1));
	}
}
int main(int argc,char* argv[])
{
	//忽略管道信号，发送数据给已关闭的socket
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	event_config* conf=event_config_new();
	//设置支持文件描述符
	event_config_require_features(conf,EV_FEATURE_FDS);
	event_base* base=event_base_new_with_config(conf);
	event_config_free(conf);
	if(!base)
	{
		cout<<"event_base_new_wiht_config failed\n";
		return -1;
	}
	//打开文件只读非阻塞
	int sock=open("/var/log/auth.log",O_RDONLY|O_NONBLOCK,0);
	if(sock<=0)
	{
		cerr<<"/var/log/auth.log open failed\n";
		return -2;
	}
	//文件指针移到结尾处
	lseek(sock,0,SEEK_END);
	
	//监听文件数据
	event* fev=event_new(base,sock,EV_READ|EV_PERSIST,read_file,0);
	event_add(fev,NULL);
	//进入事件主循环
	event_base_dispatch(base);
	event_base_free(base);
	
	return 0;
	
}
//man open
//man 2 open
