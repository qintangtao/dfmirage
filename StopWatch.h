#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

class StopWatch
{
private:
    LONGLONG _freq;
    LARGE_INTEGER _begin;
    LARGE_INTEGER _end;

public:
    double costTime;

    StopWatch(void)
    {
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency(&tmp);
        _freq = tmp.QuadPart;
        costTime = 0;
    }

    ~StopWatch(void)
    {

    }

    void Start()            // 开始计时
    {
        QueryPerformanceCounter(&_begin);
    }

    void End()                // 结束计时
    {
        QueryPerformanceCounter(&_end);
        costTime = ((_end.QuadPart - _begin.QuadPart)*1000000.0f / _freq);
        //printf("_begin:%I64d, _end:%I64d, costTime:%f\n",  _begin.QuadPart, _end.QuadPart, costTime);
    }

    void Reset()            // 计时清0
    {
        costTime = 0;
    }

    static void SleepPerformUS(DWORD usec)
    {
        LARGE_INTEGER perfCnt, start, now;

        QueryPerformanceFrequency(&perfCnt);
        QueryPerformanceCounter(&start);

        do {
            QueryPerformanceCounter((LARGE_INTEGER*)&now);
        } while ((now.QuadPart - start.QuadPart) / double(perfCnt.QuadPart) * 1000000 < usec);
    }
};

#endif // STOPWATCH_H
