#pragma once

#include <cstdint>

#include <Arduino.h>

namespace app {

/*
 * Wall-clock time source for a continuously running device.
 *
 * The board has no RTC, so the UNIX epoch is fetched from the CoAP config
 * server (see Coap) and anchored to millis(). now() then extrapolates the
 * current epoch from the elapsed uptime. Re-sync periodically to correct
 * drift and to survive the ~49.7 day millis() rollover.
 */
class Clock
{
public:
    static Clock& getInstance()
    {
        static Clock clock;
        return clock;
    }

    // Anchor the given UNIX epoch (seconds) to the current uptime.
    void sync(uint32_t epoch)
    {
        epoch_ = epoch;
        anchorMs_ = millis();
    }

    bool isSynced() const { return epoch_ != 0; }

    // Current UNIX epoch (seconds), or 0 when never synced.
    uint32_t now() const
    {
        if (!epoch_)
        {
            return 0;
        }

        return epoch_ + (millis() - anchorMs_) / 1000;
    }

private:
    Clock() = default;

    uint32_t epoch_ = 0;
    uint32_t anchorMs_ = 0;
};

}  // namespace app
