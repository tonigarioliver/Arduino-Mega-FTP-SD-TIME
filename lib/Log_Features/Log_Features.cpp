#include "Log_Features.h"
#include "Arduino.h"
#include <stdio.h>
#include <string.h>


Log_Features::Log_Features(int deltatimelog, unsigned long long lastlog){
    _deltatimelog = deltatimelog;
    _lastlog = lastlog;
};

void Log_Features::set_deltatimelog(int deltatimelog){
    _deltatimelog = deltatimelog;
};

void Log_Features::set_lastlog(unsigned long long lastlog){
    _lastlog = lastlog;
};

unsigned long long Log_Features::get_lastlog(){
    return _lastlog;
};

int Log_Features::get_deltatimelog(){
    return _deltatimelog;
};

bool Log_Features::enablelog(unsigned long long currentTime){
    if((currentTime -_lastlog)>=_deltatimelog){
        _lastlog = currentTime;
        return true;
    }
    return false;
}