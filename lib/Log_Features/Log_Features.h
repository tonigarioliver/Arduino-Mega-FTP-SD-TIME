#include "Arduino.h"

class Log_Features{
    private:
        int _deltatimelog;
        unsigned long long _lastlog;
    public:
        Log_Features(int deltatimelog, unsigned long long lastlog);

        void set_deltatimelog(int deltatimelog);
        void set_lastlog(unsigned long long lastlog);
        int get_deltatimelog();
        unsigned long long get_lastlog();
        bool enablelog(unsigned long long currentTime);

};