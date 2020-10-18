#include "pch.h"
#include "ThreadPool.h"

Task::Task(LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	proc = threadProc;
	param = lpParam;
}

Thread::Thread(Task* task) {
	execTask = task;
	InitializeCriticalSection(&cs);
	InitializeConditionVariable(&cv);
}

DWORD WINAPI ThreadPool::ManagerThreadStart(LPVOID lpParam) {
	return ((ThreadPool*)lpParam)->ManagerThreadMain();
}

DWORD ThreadPool::ManagerThreadMain() {
	EnterCriticalSection(&csThreadQueue);
	while (taskQueue.size() < initThreadAmount)
		SleepConditionVariableCS(&cvThreadQueue, &csThreadQueue, INFINITE);
	LeaveCriticalSection(&csThreadQueue);

	while (notDestructed) {
		EnterCriticalSection(&csTaskQueue);
		while (taskQueue.empty())
			SleepConditionVariableCS(&cvTaskQueue, &csTaskQueue, INFINITE);
		Task* task = taskQueue.front();
		taskQueue.pop();
		LeaveCriticalSection(&csTaskQueue);

		EnterCriticalSection(&csThreadQueue);
		if (runningThreadAmount < threads.size()) {
			while (threadQueue.front()->execTask != NULL) {
				Thread* thread = threadQueue.front();
				threadQueue.pop();
				threadQueue.push(thread);
			}
		}
		else {
			threads.push_back(CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL));
			SleepConditionVariableCS(&cvThreadQueue, &csThreadQueue, INFINITE);
		}
		threadQueue.back()->execTask = task;
		LeaveCriticalSection(&csThreadQueue);
	}
	return 0;
}

DWORD WINAPI ThreadPool::ThreadStart(LPVOID lpParam) {
	return ((ThreadPool*)lpParam)->ThreadMain();
}

DWORD ThreadPool::ThreadMain() {
	Thread* thread = new Thread();
	EnterCriticalSection(&csThreadQueue);
	threadQueue.push(thread);
	WakeConditionVariable(&cvThreadQueue);
	LeaveCriticalSection(&csThreadQueue);

	while (notDestructed) {	
		EnterCriticalSection(&thread->cs);
		while (thread->execTask == NULL)
			SleepConditionVariableCS(&thread->cv, &thread->cs, INFINITE);
		thread->execTask->proc(thread->execTask->param);
		delete thread->execTask;
		runningThreadAmount--;
		LeaveCriticalSection(&thread->cs);
	}
	return 0;
}

VOID ThreadPool::AddTask(LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	EnterCriticalSection(&csTaskQueue);
	taskQueue.push(new Task(threadProc, lpParam));
	WakeConditionVariable(&cvTaskQueue);
	LeaveCriticalSection(&csTaskQueue);
}

ThreadPool::ThreadPool(int maxAmount) {
	notDestructed = true;
	initThreadAmount = 3;
	maxThreadAmount = maxAmount;
	runningThreadAmount = 0;

	InitializeCriticalSection(&csTaskQueue);
	InitializeCriticalSection(&csThreadQueue);
	InitializeConditionVariable(&cvTaskQueue);
	InitializeConditionVariable(&cvThreadQueue);

	for (int i = 0; i < initThreadAmount; i++)
		threads.push_back(CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL));
	managerThread = CreateThread(NULL, 0, ManagerThreadStart, (LPVOID)this, 0, NULL);
	
}

ThreadPool::~ThreadPool() {
	notDestructed = false;
	WakeAllConditionVariable(&cvTaskQueue);
	WakeAllConditionVariable(&cvThreadQueue);
	WaitForSingleObject(managerThread, INFINITE);
	for (int i = 0; i < threads.size(); i++)
	{
		WaitForSingleObject(threads[i], INFINITE);
	}
	DeleteCriticalSection(&csTaskQueue);
	DeleteCriticalSection(&csThreadQueue);

	CloseHandle(managerThread);
	for (int i = 0; i < threads.size(); i++) {
		CloseHandle(threads[i]);
	}
}