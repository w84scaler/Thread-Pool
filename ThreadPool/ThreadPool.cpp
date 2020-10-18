#include "pch.h"
#include "ThreadPool.h"

Task::Task(LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	proc = threadProc;
	param = lpParam;
}

Thread::Thread() {
	execTask = NULL;
	InitializeCriticalSection(&cs);
	InitializeConditionVariable(&cv);
}

Thread::~Thread() {

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
		if (runningThreadAmount < threads.size()) {
			while (threadQueue.front()->execTask != NULL) {
				Thread* thread = threadQueue.front();
				threadQueue.pop();
				threadQueue.push(thread);
			}
			threadQueue.front()->execTask = task;
			WakeConditionVariable(&threadQueue.front()->cv);
		}
		else {
			threads.push_back(CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL));
			SleepConditionVariableCS(&cvThreadQueue, &csThreadQueue, INFINITE);
			threadQueue.back()->execTask = task;
			WakeConditionVariable(&threadQueue.back()->cv);
		}
		runningThreadAmount++;
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

	while (!Destructed) {	
		EnterCriticalSection(&thread->cs);
		while (thread->execTask == NULL && !Destructed)
			SleepConditionVariableCS(&thread->cv, &thread->cs, INFINITE);
		if (Destructed) {
			LeaveCriticalSection(&thread->cs);
			return 0;
		}
		Task* task = thread->execTask;
		thread->execTask = NULL;
		runningThreadAmount--;
		LeaveCriticalSection(&thread->cs);
		task->proc(task->param);
		delete task;
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
	Destructed = false;
	initThreadAmount = 3;
	maxThreadAmount = maxAmount;
	runningThreadAmount = 0;

	InitializeCriticalSection(&csTaskQueue);
	InitializeCriticalSection(&csThreadQueue);
	InitializeConditionVariable(&cvTaskQueue);
	InitializeConditionVariable(&cvThreadQueue);

	managerThread = CreateThread(NULL, 0, ManagerThreadStart, (LPVOID)this, 0, NULL);
	for (int i = 0; i < initThreadAmount; i++)
		threads.push_back(CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL));
}

ThreadPool::~ThreadPool() {
	Sleep(100);
	Destructed = true;
	WakeAllConditionVariable(&cvTaskQueue);
	WakeAllConditionVariable(&cvThreadQueue);
	WaitForSingleObject(managerThread, INFINITE);
	for (int i = 0; i < threadQueue.size(); i++) {
		Thread* thread = threadQueue.front();
		WakeAllConditionVariable(&thread->cv);
		threadQueue.pop();
		threadQueue.push(thread);
	}
	for (int i = 0; i < threads.size(); i++) {
		WaitForSingleObject(threads[i], INFINITE);
	}
	while (!threadQueue.empty()) {
		DeleteCriticalSection(&threadQueue.back()->cs);
		threadQueue.pop();
	}
	DeleteCriticalSection(&csTaskQueue);
	DeleteCriticalSection(&csThreadQueue);

	CloseHandle(managerThread);
	for (int i = 0; i < threads.size(); i++) {
		CloseHandle(threads[i]);
	}
}