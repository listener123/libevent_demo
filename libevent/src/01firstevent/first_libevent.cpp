#include <event2/event.h>
#include <iostream>
using namespace std;
int main()
{
	cout<<"helloworld"<<endl;
	event_base *base=event_base_new();
	if(base)
	{
		cout<<"new_success"<<endl;
	}
	return 0;
}
