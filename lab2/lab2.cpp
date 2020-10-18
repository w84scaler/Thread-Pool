#include "lab2.h"

DWORD WINAPI MyFunc(LPVOID lpParam)
{
    std::string str = std::to_string(GetCurrentThreadId()) + "\n";
    std::cout << str << std::flush;
    return 0;
}

int main()
{
    ThreadPool* threadPool = new ThreadPool(5);
    for (int i = 0; i < 15; i++)
    {
        threadPool->AddTask(MyFunc);
    }
    threadPool->~ThreadPool();
}