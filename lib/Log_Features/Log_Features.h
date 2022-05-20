#include "Arduino.h"

class Log_Features
{
private:
    unsigned long _deltatimelog; //// miliseconds
    unsigned long long _lastlog;
    int _percentatgelog;
    float _lasttemplog;

public:
    Log_Features(int deltatimelog, unsigned long long lastlog, int percentatgelog, float lasttemplog); //// pass seconds

    void set_deltatimelog(int deltatimelog);
    void set_lastlog(unsigned long long lastlog);
    void set_percentatgelog(int percentatgelog);
    void set_lasttemplog(float lasttemplog);
    float get_lasttemplog();
    int get_percentatgelog();
    int get_deltatimelog();
    unsigned long long get_lastlog();
    bool enablelog(unsigned long long currentTime, float currentTemperature);
};