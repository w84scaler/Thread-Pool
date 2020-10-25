#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>

class ThreadPoolLogger {
public:
	ThreadPoolLogger(std::string path);
	~ThreadPoolLogger();

	CRITICAL_SECTION cs;

	void PoolCreated(int initAmount, int maxAmount);
	void PoolDestroyed();
	void TaskSkipped(int taskNum);
	void TaskAdded(int taskNum);
	void TaskThrowedException(int threadID, const char* info);
	void ThreadAdded(int threadID);
	void ThreadGotTask(int threadID, int taskID);
	void ThreadFreed(int threadID, int taskID);
	void ThreadDestroyed(int threadID);

private:
	std::string filePath;
	std::ofstream fileObj;
	char* GetTime();
};
