#include <Arduino.h>
#include <algorithm>

#include "TimerDispatcher.h"

namespace app {

// --------------------------------------------------------

TimerDispatcher::TimerDispatcher()
  : mLastTimeMs(millis())
{}

void TimerDispatcher::startTimer(TimerListener* listener, msec interval, bool runOnce)
{
    if (!listener) return;

    // Set empty deleter for raw pointers
    auto sharedListener = std::shared_ptr<TimerListener>(listener, [](TimerListener *) {});
    startTimer(sharedListener, interval, runOnce);
}

void TimerDispatcher::startTimer(std::shared_ptr<TimerListener> listener, msec interval, bool runOnce)
{
    if (!listener) return;

    Timer timer;
    timer.listener = listener;
    timer.nextTime = millis() + interval;
    timer.interval = runOnce ? TIME_INVALID : interval;

    mTimers.push_back(timer);
}

void TimerDispatcher::startTimer(std::function<void()> callback, msec interval, bool runOnce)
{
    if (!callback) return;

    auto listener = std::make_shared<CallbackTimerListener>(std::move(callback));
    startTimer(listener, interval, runOnce);
}

void TimerDispatcher::process()
{
    msec currentTimeMs = millis();

    auto it = mTimers.begin();
    while (it != mTimers.end())
    {
        if (currentTimeMs >= it->nextTime)
        {
            if (it->listener)
            {
                it->listener->onTimer();
            }

            if (it->interval == TIME_INVALID)
            {
                it = mTimers.erase(it);
            }
            else
            {
                it->nextTime += it->interval;

                if (it->nextTime < currentTimeMs)
                {
                    it->nextTime = currentTimeMs + it->interval;
                }

                ++it;
            }
        }
        else
        {
            ++it;
        }
    }

    if (mLastTimeMs + 60000 < currentTimeMs)
    {
        mLastTimeMs = currentTimeMs;
    }
}

void TimerDispatcher::clear()
{
    mTimers.clear();
}

} // namespace app