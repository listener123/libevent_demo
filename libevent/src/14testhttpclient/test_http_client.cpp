#include <event2/event.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <string>
using namespace std;
void http_client_cb(struct evhttp_request* req,void* ctx)
{
	cout<<"http_client_cb"<<endl;
	event_base* base=(event_base*)ctx;
	//服务端响应错误
	if(req==NULL)
	{
		int errocode=EVUTIL_SOCKET_ERROR();
		cout<<"socket error: "<<evutil_socket_error_to_string(errocode)<<endl;
		return;
	}
	//获取path
	const char* path=evhttp_request_get_uri(req);
	cout<<"request path: "<<path<<endl;
	string filepath=".";
	filepath+=path;
	cout<<"filepath is "<<filepath<<endl;
	//如果路径中有目标，需要分析出目录，并创建
	FILE* fp=fopen(filepath.c_str(),"wb");
	if(!fp)
	{
		cout<<"open file "<<filepath<<" failed"<<endl;
	}
	//获取返回的code 200 404
	cout<<"Response: "<<evhttp_request_get_response_code(req);
	cout<<" "<<evhttp_request_get_response_code_line(req)<<endl;
	char buf[1024]={0};
	evbuffer* input=evhttp_request_get_input_buffer(req);
	for(;;)
	{
		int len=evbuffer_remove(input,buf,sizeof(buf)-1);
		if(len<=0)break;
		buf[len]=0;
		if(!fp) continue;
		
		fwrite(buf,1,len,fp);
	}
	if(fp)fclose(fp);
	
	event_base_loopbreak(base);
}
int TestPosthttp()
{
	event_base* base=event_base_new();
	if(base)
	{
		cout<<"event_base_new success"<<endl;
	}
	//生成请求信息 POST
	string http_url="http://127.0.0.1:8080/index.html?id=1";
	//分析url地址
	//uri
	evhttp_uri* uri=evhttp_uri_parse(http_url.c_str());
	//http https
	const char* scheme=evhttp_uri_get_scheme(uri);
	if(!scheme)
	{
		cerr<<"scheme is null"<<endl;
		return -1;
	}
	cout<<"scheme: "<<scheme<<endl;
	int port=evhttp_uri_get_port(uri);
	if(port<0)port=80;
	cout<<"port: "<<port<<endl;
	//host 127.0.0.1
	const char* host=evhttp_uri_get_host(uri);
	if(!host)
	{
		cerr<<"host is null"<<endl;
		return -1;
	}
	cout<<"host: "<<host<<endl;
	const char* path=evhttp_uri_get_path(uri);
	if(!path||strlen(path)==0)
	{
		path="/";
	}
	if(path) cout<<"path: "<<path<<endl;
	//?后面的内容 id=1
	const char* query=evhttp_uri_get_query(uri);
	if(query)
	{
		cout<<"query: "<<query<<endl;
	}
	else cout<<"query: null";
	
	
	//bufferevent 连接http服务器
	bufferevent* bev=bufferevent_socket_new(base,-1,BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon=evhttp_connection_base_bufferevent_new(base,
		NULL,bev,host,port);
	//http client 请求
	evhttp_request* req=evhttp_request_new(http_client_cb,base);
	//设置请求的head消息报头 信息
	evkeyvalq* output_headers=evhttp_request_get_output_headers(req);
	evhttp_add_header(output_headers,"Host",host);
	//发送post数据
	evbuffer*output=evhttp_request_get_output_buffer(req);
	evbuffer_add_printf(output,"username=%d&keyword=%d",1,2);
	//发起请求
	evhttp_make_request(evcon,req,EVHTTP_REQ_POST,path);
	//事件分发处理
	if(base)
		event_base_dispatch(base);
	if(base)
		event_base_free(base);
	return 0;
}
int TestGethttp()
{
	event_base* base=event_base_new();
	if(base)
	{
		cout<<"event_base_new success"<<endl;
	}
	//生成请求信息 GET
	string http_url="http://127.0.0.1:8080/test.png";
	//分析url地址
	//uri
	evhttp_uri* uri=evhttp_uri_parse(http_url.c_str());
	//http https
	const char* scheme=evhttp_uri_get_scheme(uri);
	if(!scheme)
	{
		cerr<<"scheme is null"<<endl;
		return -1;
	}
	cout<<"scheme: "<<scheme<<endl;
	int port=evhttp_uri_get_port(uri);
	if(port<0)port=80;
	cout<<"port: "<<port<<endl;
	//host ffmpeg.club
	const char* host=evhttp_uri_get_host(uri);
	if(!host)
	{
		cerr<<"host is null"<<endl;
		return -1;
	}
	cout<<"host: "<<host<<endl;
	const char* path=evhttp_uri_get_path(uri);
	if(!path||strlen(path)==0)
	{
		path="/";
	}
	if(path) cout<<"path: "<<path<<endl;
	//?后面的内容 id=1
	const char* query=evhttp_uri_get_query(uri);
	if(query)
	{
		cout<<"query: "<<query<<endl;
	}
	else cout<<"query: null";
	
	
	//bufferevent 连接http服务器
	bufferevent* bev=bufferevent_socket_new(base,-1,BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon=evhttp_connection_base_bufferevent_new(base,
		NULL,bev,host,port);
	//http client 请求
	evhttp_request* req=evhttp_request_new(http_client_cb,base);
	//设置请求的head消息报头 信息
	evkeyvalq* output_headers=evhttp_request_get_output_headers(req);
	evhttp_add_header(output_headers,"Host",host);
	//发起请求
	evhttp_make_request(evcon,req,EVHTTP_REQ_GET,path);
	//事件分发处理
	if(base)
		event_base_dispatch(base);
	if(base)
		event_base_free(base);
	return 0;
}
int main()
{
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
		return 1;
	cout<<"test server\n";
	TestGethttp();
	TestPosthttp();
	return 0;
	
}
