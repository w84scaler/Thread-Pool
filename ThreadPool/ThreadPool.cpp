#include "pch.h"
#include "ThreadPool.h"

Task::Task(LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	proc = threadProc;
	param = lpParam;
}

Thread::Thread() {
	task = NULL;
	handler = OpenThread(THREAD_ALL_ACCESS, true, GetCurrentThreadId());
	InitializeCriticalSection(&cs);
	InitializeConditionVariable(&cv);
}

Thread::~Thread() {
	WakeAllConditionVariable(&cv);
	WaitForSingleObject(handler, INFINITE);
	DeleteCriticalSection(&cs);
	CloseHandle(handler);
}

DWORD WINAPI ThreadPool::ManagerThreadStart(LPVOID lpParam) {
	return ((ThreadPool*)lpParam)->ManagerThreadMain();
}

DWORD ThreadPool::ManagerThreadMain() {
	EnterCriticalSection(&csThreadQueue);
	while (threadQueue.size() < initThreadAmount)
		SleepConditionVariableCS(&cvThreadQueue, &csThreadQueue, INFINITE);
	LeaveCriticalSection(&csThreadQueue);

	while (!Destructed) {
		EnterCriticalSection(&csTaskQueue);
		while (taskQueue.empty() && !Destructed)
			SleepConditionVariableCS(&cvTaskQueue, &csTaskQueue, INFINITE);
		if (Destructed) {
			LeaveCriticalSection(&csTaskQueue);
			return 0;
		}
		Task* task = taskQueue.front();
		taskQueue.pop();
		LeaveCriticalSection(&csTaskQueue);

		EnterCriticalSection(&csThreadQueue);
		Thread* thread;
		if (workThreadAmount < threadQueue.size() || maxThreadAmount == threadQueue.size()) {
			while (threadQueue.front()->task != NULL) {
				Thread* stillWorkThread = threadQueue.front();
				threadQueue.pop();
				threadQueue.push(stillWorkThread);
			}
			thread = threadQueue.front();
		}
		else {
			CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL);
			SleepConditionVariableCS(&cvThreadQueue, &csThreadQueue, INFINITE);
			thread = threadQueue.back();
		}
		LeaveCriticalSection(&csThreadQueue);

		EnterCriticalSection(&csWorkThreadAmount);
		workThreadAmount++;
		LeaveCriticalSection(&csWorkThreadAmount);

		EnterCriticalSection(&thread->cs);
		thread->task = task;
		WakeConditionVariable(&thread->cv);
		LeaveCriticalSection(&thread->cs);
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
	while (!Destructed) {	
		EnterCriticalSection(&thread->cs);
		while (thread->task == NULL && !Destructed)
			SleepConditionVariableCS(&thread->cv, &thread->cs, INFINITE);
		if (Destructed) {
			LeaveCriticalSection(&thread->cs);
			return 0;
		}
		Task* task = thread->task;
		thread->task = NULL;
		LeaveCriticalSection(&thread->cs);

		task->proc(task->param);
		delete task;

		EnterCriticalSection(&csWorkThreadAmount);
		workThreadAmount--;
		LeaveCriticalSection(&csWorkThreadAmount);
	}
	return 0;
}

BOOL ThreadPool::AddTask(LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	bool isFull = false;
	EnterCriticalSection(&csWorkThreadAmount);
	if (workThreadAmount == maxThreadAmount)
		isFull = true;
	LeaveCriticalSection(&csWorkThreadAmount);

	if (!isFull) {
		EnterCriticalSection(&csTaskQueue);
		taskQueue.push(new Task(threadProc, lpParam));
		WakeConditionVariable(&cvTaskQueue);
		LeaveCriticalSection(&csTaskQueue);
		return true;
	}
	else {
		return false;
	}
}

ThreadPool::ThreadPool(int maxAmount) {
	Destructed = false;
	initThreadAmount = 3;
	maxThreadAmount = maxAmount;
	workThreadAmount = 0;

	InitializeCriticalSection(&csWorkThreadAmount);
	InitializeCriticalSection(&csTaskQueue);
	InitializeCriticalSection(&csThreadQueue);
	InitializeConditionVariable(&cvTaskQueue);
	InitializeConditionVariable(&cvThreadQueue);

	managerThread = CreateThread(NULL, 0, ManagerThreadStart, (LPVOID)this, 0, NULL);
	for (int i = 0; i < initThreadAmount; i++)
		CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL);
}

ThreadPool::~ThreadPool() {
	Destructed = true;
	WakeAllConditionVariable(&cvTaskQueue);
	WakeAllConditionVariable(&cvThreadQueue);
	WaitForSingleObject(managerThread, INFINITE);
	while (!threadQueue.empty()) {
		Thread* thread = threadQueue.front();
		threadQueue.pop();
		thread->~Thread();
	}
	DeleteCriticalSection(&csTaskQueue);
	DeleteCriticalSection(&csThreadQueue);
	DeleteCriticalSection(&csWorkThreadAmount);
}