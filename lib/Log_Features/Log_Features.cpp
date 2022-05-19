#include "Log_Features.h"
#include "Arduino.h"
#include <stdio.h>
#include <string.h>

Log_Features::Log_Features(int deltatimelog, unsigned long long lastlog, int percentatgelog, float lasttemplog)
{
    _deltatimelog = deltatimelog;
    _lastlog = lastlog;
    _lasttemplog = lasttemplog;
    _percentatgelog = percentatgelog;
};

void Log_Features::set_deltatimelog(int deltatimelog)
{
    _deltatimelog = deltatimelog;
};

void Log_Features::set_lastlog(unsigned long long lastlog)
{
    _lastlog = lastlog;
};

unsigned long long Log_Features::get_lastlog()
{
    return _lastlog;
};

int Log_Features::get_deltatimelog()
{
    return _deltatimelog;
};

void Log_Features::set_percentatgelog(int percentatgelog)
{
    _percentatgelog = percentatgelog;
}

void Log_Features::set_lasttemplog(float lasttemplog)
{
    _lasttemplog = lasttemplog;
}

float Log_Features::get_lasttemplog()
{
    return _lasttemplog;
}
int Log_Features::get_percentatgelog()
{
    return _percentatgelog;
}

bool Log_Features::enablelog(unsigned long long currentTime)
{
    if ((currentTime - _lastlog) >= _deltatimelog)
    {
        _lastlog = currentTime;
        return true;
    }
    return false;
}