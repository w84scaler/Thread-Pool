#include "pch.h"
#include "ThreadPool.h"

Task::Task(int taskID, LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	proc = threadProc;
	param = lpParam;
	id = taskID;
}

Thread::Thread() {
	task = NULL;
	id = GetCurrentThreadId();
	handler = OpenThread(THREAD_ALL_ACCESS, true, id);
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
		Thread* thread = NULL;

		/*EnterCriticalSection(&csWorkThreadAmount);
		int work = workThreadAmount;
		LeaveCriticalSection(&csWorkThreadAmount);*/

		if (workThreadAmount < threadQueue.size() || maxThreadAmount == threadQueue.size()) {
			if (workThreadAmount < threadQueue.size()) {
				while (threadQueue.front()->task != NULL) {
					Thread* stillWorkThread = threadQueue.front();
					threadQueue.pop();
					threadQueue.push(stillWorkThread);
				}
				thread = threadQueue.front();
			}
			else {
				EnterCriticalSection(&logger->cs);
				logger->TaskSkipped(task->id);
				LeaveCriticalSection(&logger->cs);
			}
		}
		else {
			CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL);
			SleepConditionVariableCS(&cvThreadQueue, &csThreadQueue, INFINITE);
			thread = threadQueue.back();
			EnterCriticalSection(&logger->cs);
			logger->ThreadAdded(thread->id);
			LeaveCriticalSection(&logger->cs);
		}
		LeaveCriticalSection(&csThreadQueue);

		if (thread != NULL) {
			EnterCriticalSection(&csWorkThreadAmount);
			workThreadAmount++;
			LeaveCriticalSection(&csWorkThreadAmount);

			EnterCriticalSection(&thread->cs);
			thread->task = task;
			WakeConditionVariable(&thread->cv);
			LeaveCriticalSection(&thread->cs);
		}
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
		LeaveCriticalSection(&thread->cs);
		
		EnterCriticalSection(&logger->cs);
		logger->ThreadGotTask(thread->id, task->id);
		LeaveCriticalSection(&logger->cs);

		try
		{
			task->proc(task->param);
		}
		catch (const char* e)
		{
			EnterCriticalSection(&logger->cs);
			logger->TaskThrowedException(thread->id, e);
			LeaveCriticalSection(&logger->cs);
		}

		EnterCriticalSection(&thread->cs);
		thread->task = NULL;
		LeaveCriticalSection(&thread->cs);

		EnterCriticalSection(&logger->cs);
		logger->ThreadFreed(thread->id, task->id);
		LeaveCriticalSection(&logger->cs);

		delete task;

		EnterCriticalSection(&csWorkThreadAmount);
		workThreadAmount--;
		LeaveCriticalSection(&csWorkThreadAmount);
	}
	return 0;
}

BOOL ThreadPool::AddTask(LPTHREAD_START_ROUTINE threadProc, LPVOID lpParam) {
	taskAmount++;

	EnterCriticalSection(&csWorkThreadAmount);
	int work = workThreadAmount;
	LeaveCriticalSection(&csWorkThreadAmount);
	bool isFull = false;
	if (work == maxThreadAmount)
		isFull = true;

	if (!isFull) {
		EnterCriticalSection(&csTaskQueue);
		taskQueue.push(new Task(taskAmount, threadProc, lpParam));
		WakeConditionVariable(&cvTaskQueue);
		LeaveCriticalSection(&csTaskQueue);
		EnterCriticalSection(&logger->cs);
		logger->TaskAdded(taskAmount);
		LeaveCriticalSection(&logger->cs);
		return true;
	}
	else {
		EnterCriticalSection(&logger->cs);
		logger->TaskSkipped(taskAmount);
		LeaveCriticalSection(&logger->cs);
		return false;
	}
}

ThreadPool::ThreadPool(int maxAmount) {
	logger = new ThreadPoolLogger("D:\\log.txt");

	Destructed = false;
	initThreadAmount = 3;
	maxThreadAmount = maxAmount;
	workThreadAmount = 0;
	taskAmount = 0;

	InitializeCriticalSection(&csWorkThreadAmount);
	InitializeCriticalSection(&csTaskQueue);
	InitializeCriticalSection(&csThreadQueue);
	InitializeConditionVariable(&cvTaskQueue);
	InitializeConditionVariable(&cvThreadQueue);

	managerThread = CreateThread(NULL, 0, ManagerThreadStart, (LPVOID)this, 0, NULL);
	for (int i = 0; i < initThreadAmount; i++)
		CreateThread(NULL, 0, ThreadStart, (LPVOID)this, 0, NULL);
	EnterCriticalSection(&logger->cs);
	logger->PoolCreated(initThreadAmount, maxThreadAmount);
	LeaveCriticalSection(&logger->cs);
}

ThreadPool::~ThreadPool() {
	Destructed = true;
	WakeAllConditionVariable(&cvTaskQueue);
	WakeAllConditionVariable(&cvThreadQueue);
	WaitForSingleObject(managerThread, INFINITE);
	while (!threadQueue.empty()) {
		Thread* thread = threadQueue.front();
		threadQueue.pop();
		int id = thread->id;
		thread->~Thread();
		logger->ThreadDestroyed(id);
	}
	DeleteCriticalSection(&csTaskQueue);
	DeleteCriticalSection(&csThreadQueue);
	DeleteCriticalSection(&csWorkThreadAmount);
	logger->PoolDestroyed();
	logger->~ThreadPoolLogger();
}