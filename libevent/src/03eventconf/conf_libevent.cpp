#include <event2/event.h>
#include <signal.h>
#include <string.h>
#include <iostream>
using namespace std;
#define SPORT 5000

int main()
{
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	cout<<"test server\n";
	
	
	//创建配置上下文
	event_config* conf=event_config_new();
	
	//显示支持的网络模式
	const char** methods=event_get_supported_methods();
	cout<<"supported methods\n";
	for(int i=0;methods[i]!=NULL;i++)
	{
		cout<<methods[i]<<endl;
	}
	//设置特征
	//evnet_config_require_features(conf,EV_FEATURE_ET|EV_FEATURE_FDS);
	//evnet_config_require_features(conf,EV_FEATURE_FDS);
	
	//设置网络模型，使用select
	event_config_avoid_method(conf,"epoll");
	event_config_avoid_method(conf,"poll");
	
	//初始化配置libevent上下文
	event_base* base=event_base_new_with_config(conf);
	
	event_config_free(conf);
	if(!base)
	{
		cerr<<"event_base_new_with_config failure\n";
		base = event_base_new();
	if(!base)
	{
		cerr<<"event_base_new failed\n";
		return 0;
	}
	}
	else
	{
		//获取当前网络模型
		cout<<"current method is "<<event_base_get_method(base)<<endl;
		
		//确认特征是否生效
	    int f = event_base_get_features(base);
		if(f&EV_FEATURE_ET)cout<<"EV_FEATURE_ET are supported."<<endl;
		else cout<<"EV_FEATURE_ET are not supported."<<endl;
		if(f&EV_FEATURE_O1)cout<<"EV_FEATURE_01 are supported."<<endl;
		else cout<<"EV_FEATURE_01 are not supported."<<endl;
		if(f&EV_FEATURE_FDS)cout<<"EV_FEATURE_FDS are supported."<<endl;
		else cout<<"EV_FEATURE_FDS are not supported."<<endl;
		if(f&EV_FEATURE_EARLY_CLOSE)cout<<"EV_FEATURE_EARLY_CLOSE are supported."<<endl;
		else cout<<"EV_FEATURE_EARLY_CLOSE are not supported."<<endl;
		cout<<"event_base_new_with_config success\n";
		event_base_free(base);
	}
	return 0;
	
}
