#pragma once

#ifdef THREADPOOL_EXPORTS
#define THREADPOOL_API __declspec(dllexport)
#else
#define THREADPOOL_API __declspec(dllimport)
#endif

#include <queue>
#include "Logger.h"

class Task {
public:
	Task(int taskID, LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam);

	LPTHREAD_START_ROUTINE proc;
	LPVOID param;
	int id;
};

class Thread {
public:
	Thread();
	~Thread();

	int id;
	Task* task;
	HANDLE handler;
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cv;
};

extern "C" class THREADPOOL_API ThreadPool {
private:
	bool Destructed;

	ThreadPoolLogger* logger;

	CRITICAL_SECTION csWorkThreadAmount;

	CRITICAL_SECTION csTaskQueue;
	CRITICAL_SECTION csThreadQueue;
	CONDITION_VARIABLE cvTaskQueue;
	CONDITION_VARIABLE cvThreadQueue;

	std::queue<Task*> taskQueue;
	std::queue<Thread*> threadQueue;

	int initThreadAmount;
	int maxThreadAmount;
	int workThreadAmount;

	int taskAmount;

	HANDLE managerThread;

	DWORD ManagerThreadMain();
	DWORD ThreadMain();

	static DWORD WINAPI ManagerThreadStart(LPVOID lpParam);
	static DWORD WINAPI ThreadStart(LPVOID lpParam);

public:
	ThreadPool(int maxAmount);
	~ThreadPool();
	BOOL AddTask(LPTHREAD_START_ROUTINE ThreadProc, LPVOID lpParam = NULL);
};