#pragma once

#ifdef THREADPOOL_EXPORTS
#define THREADPOOL_API extern "C" __declspec(dllexport)
#else
#define THREADPOOL_API extern "C" __declspec(dllimport)
#endif

THREADPOOL_API class ThreadPool {
private:

public:
	ThreadPool(int MaxThreadAmount) {}
	~ThreadPool() {}
	BOOL AddTask(LPTHREAD_START_ROUTINE ThreadProc, LPVOID lpParam = NULL) {}
};