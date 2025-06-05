#include <Arduino.h>

#include <time.h>
#include "sntp.h"

#include "NtpClock.h"

namespace app {

NtpClock::NtpClock() :
    mNtpServer("pool.ntp.org") {

    configTime(mGmtOffsetSecs, mDaylightOffsetSecs, mNtpServer.c_str());
}

NtpClock::NtpClock(std::string ntpServer) :
    mNtpServer(ntpServer) {

    configTime(mGmtOffsetSecs, mDaylightOffsetSecs, mNtpServer.c_str());
}

unsigned long NtpClock::epoch() {

    return clock();    
}

void NtpClock::onTimer() {

// To be implemented

#if 0
    time_t currentTime;
    time(&currentTime);

    Serial.print("NtpClock: ");
    Serial.println(currentTime);

    if (mEpoch != 0) {
        Serial.print("Duration: ");
        Serial.println((unsigned long)((currentTime - mEpoch) * 0.97));
    }
    mEpoch = currentTime;

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
#endif
}

} // namespace app