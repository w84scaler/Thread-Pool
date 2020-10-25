#include "lab2.h"

DWORD WINAPI MyFunc(LPVOID lpParam)
{
    Sleep(500);
    std::string str = std::to_string((int)lpParam) + " " + std::to_string(GetCurrentThreadId()) + "\n";
    std::cout << str << std::flush;
    return 0;
}

DWORD WINAPI MyExcFunc(LPVOID lpParam)
{
    Sleep(500);
    std::string str = std::to_string((int)lpParam) + " " + std::to_string(GetCurrentThreadId()) + "\n";
    std::cout << str << std::flush;
    throw "my bruh exception";
    return 0;
}

int main()
{
    ThreadPool* threadPool = new ThreadPool(4);
    int taskAmount = 0;
    if (threadPool->AddTask(MyExcFunc, 0))
        taskAmount++;
    for (int i = 1; i < 10; i++) {
        if (threadPool->AddTask(MyFunc,(LPVOID)i))
            taskAmount++;
        Sleep(100);
    }
    char ch;
    std::cin >> ch;
    threadPool->~ThreadPool();
    std::cout << std::to_string(taskAmount) << std::flush;
}