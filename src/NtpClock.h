#pragma once

#include <string>

#include <Arduino.h>

#include "TimerDispatcher.h"

namespace app {

class NtpClock : public TimerListener {

public:
    NtpClock();
    NtpClock(std::string ntpServer);

    ~NtpClock() = default;

    unsigned long epoch();

    void onTimer();

private:

    const uint32_t mGmtOffsetSecs = 3 * 3600;
    const uint32_t mDaylightOffsetSecs = 0;

    std::string mNtpServer;

    unsigned long mEpoch;
};

} // namespace app