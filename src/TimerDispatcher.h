#pragma once

#include <list>
#include <vector>
#include <functional>
#include <memory>

#include <Arduino.h>

namespace app {

using msec = unsigned long;
using secs = unsigned long;

static const msec SECONDS = 1000;
static const msec MINUTES = 60000;

static const msec TIME_INVALID = (-1);

// --------------------------------------------------------

class TimerListener {
public:
    TimerListener() {};
    virtual ~TimerListener() = default;

    virtual void onTimer() = 0;
};

class CallbackTimerListener : public TimerListener
{
public:
    explicit CallbackTimerListener(std::function<void()> callback)
        : mCallback(std::move(callback)) {}

    void onTimer() override
    {
        mCallback();
    }

private:
    std::function<void()> mCallback;
};

// --------------------------------------------------------

class TimerDispatcher {
public:
    TimerDispatcher();
    ~TimerDispatcher() = default;

    void process();

    void startTimer(TimerListener *listener, msec interval, bool runOnce = false);
    void startTimer(std::shared_ptr<TimerListener> listener, msec interval, bool runOnce = false);

    void startTimer(std::function<void()> callback, msec interval, bool runOnce = false);

    void clear();

protected:
    void startTimerImpl(std::shared_ptr<TimerListener> listener, msec interval, bool runOnce);

private:
    struct Timer {
        std::shared_ptr<TimerListener> listener;
        msec nextTime;
        msec interval;
    };

    msec mLastTimeMs;
    std::list<Timer> mTimers;
};

} // namespace app