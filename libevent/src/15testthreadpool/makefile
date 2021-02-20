testthreadpool:test_thread_pool.cpp XThreadPool.cpp XThread.cpp XFtpServerCMD.cpp
	g++ -std=c++11 $^ -o $@ -levent -lpthread
	./$@
clean:
	rm -rf testthreadpool
	rm -rf *.o
