#include <Arduino.h>

#include "TimerDispatcher.h"

namespace app {

// --------------------------------------------------------

TimerDispatcher::TimerDispatcher() :
    mLastTimeNtp((time_t)-1),
    mLastTimeNtpMs(millis()) {
}

void TimerDispatcher::startTimer(TimerListener& listener, msec interval, bool runOnce) {

    Timer timer = {
        .listener = listener,
        .nextTime = (millis() + interval),
        .interval = (runOnce) ? TIME_INVALID : interval
    };
    mTimers.push_back(timer);
}

void TimerDispatcher::process() {

    msec currentTimeMs = millis();

    for (auto it = mTimers.begin(); it != mTimers.end();) {

        if (currentTimeMs >= (*it).nextTime) {

            (*it).listener.onTimer();

            if ((*it).interval == TIME_INVALID) {
                it = mTimers.erase(it);
            }
            else {
                (*it).nextTime += (*it).interval;
            }
        }
        else {
            ++it;
        }
    }

    if (mLastTimeNtpMs + 60000 < currentTimeMs) {
        
        getTimeNtp();

        mLastTimeNtpMs = currentTimeMs;
    }
}

time_t TimerDispatcher::getTimeNtp() {
    
    time_t currentTimeNtp = time(&currentTimeNtp);
#if 0
    Serial.print("NtpClock: ");
    Serial.println(currentTimeNtp);

    if (mLastTimeNtp != 0) {
        Serial.print("Duration: ");
        Serial.println((unsigned long)((currentTimeNtp - mLastTimeNtp) * 0.97));
    }
    mLastTimeNtp = currentTimeNtp;

    struct tm * timeinfo;
    timeinfo = localtime(&currentTimeNtp);

    Serial.println(timeinfo, "%A, %B %d %Y %H:%M:%S");

    // if(!getLocalTime(&timeinfo)){
    //     Serial.println("Failed to obtain time");
    //     return;
    // }
    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
#endif
    return currentTimeNtp;
}

void TimerDispatcher::delay(msec delayMs) {
    ::delay(delayMs);
}

} // namespace app