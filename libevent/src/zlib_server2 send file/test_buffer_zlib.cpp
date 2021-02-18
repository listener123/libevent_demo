#include <event2/event.h>
#include <event2/listener.h>
#include <signal.h>
#include <string.h>
#include <iostream>
using namespace std;

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
	
	void Server(event_base* base);	
	Server(base);	
	void Client(event_base* base);	
	Client(base);	
	
	//事件分发处理
	if(base)
		event_base_dispatch(base);
	if(base)
		event_base_free(base);
	return 0;
	
}
