#include "lab2.h"

DWORD WINAPI MyFunc(LPVOID lpParam)
{
    Sleep(2000);
    std::string str = std::to_string((int)lpParam) + " " + std::to_string(GetCurrentThreadId()) + "\n";
    std::cout << str << std::flush;
    return 0;
}

int main()
{
    ThreadPool* threadPool = new ThreadPool(4);
    int taskAmount = 0;
    for (int i = 0; i < 10; i++) {
        if (threadPool->AddTask(MyFunc,(LPVOID)i))
            taskAmount++;
    }
    char ch;
    std::cin >> ch;
    threadPool->~ThreadPool();
    std::cout << std::to_string(taskAmount) << std::flush;
}