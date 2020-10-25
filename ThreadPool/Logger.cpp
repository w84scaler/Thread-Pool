#include "pch.h"
#include "Logger.h"

using namespace std;

ThreadPoolLogger::ThreadPoolLogger(std::string path) {
	InitializeCriticalSection(&cs);
	filePath = path;
	fileObj.open(filePath.c_str(), ios::out | ios::app);
}

ThreadPoolLogger::~ThreadPoolLogger() {
	DeleteCriticalSection(&cs);
	fileObj.close();
}

void ThreadPoolLogger::PoolCreated(int initAmount, int maxAmount) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "thread pool was created with " << initAmount << " threads, " << "max thread amount = " << maxAmount << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::TaskAdded(int taskNum) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "task #" << taskNum << " added" << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::TaskThrowedException(int threadID, const char* info) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - ERROR - " << "exception emerged in thread #" << threadID << ": " << info << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::TaskSkipped(int taskNum) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - WARNING - " << "pool is out of threads, task #" << taskNum << " was skipped" << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::ThreadGotTask(int threadID, int taskID) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "thread #" << threadID << " accepted the task #" << taskID << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::ThreadFreed(int threadID, int taskID) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "thread #" << threadID << " was freed after task #" << taskID << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::ThreadAdded(int threadID) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "thread #" << threadID << " added to thread pool" << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::ThreadDestroyed(int threadID) {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "thread #" << threadID << " was destroyed" << std::endl;
		fileObj << ss.str();
	}
}

void ThreadPoolLogger::PoolDestroyed() {
	if (fileObj.is_open()) {
		stringstream ss;
		ss.clear();
		ss << GetTime() << " - INFO - " << "thread pool was destroyed" << std::endl;
		fileObj << ss.str();
	}
}

char* ThreadPoolLogger::GetTime() {
	time_t now = time(NULL);
	char* currentTime = ctime(&now);
	currentTime[strlen(currentTime) - 1] = '\0';
	return currentTime;
}