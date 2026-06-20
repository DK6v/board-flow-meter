#pragma once

#include <ctime>
#include <string>

#include <WiFiManager.h>

#include "network/Clock.h"

namespace app {

/*
 * Read-only WiFiManager param-page element that displays the device's current
 * wall-clock time (fetched from the CoAP config server via Clock).
 *
 * It is constructed with no id (WiFiManagerParameter(const char*) sets _id ==
 * NULL), so WiFiManager renders it as raw custom HTML and never treats it as an
 * input or saves it. getCustomHTML() is virtual and called on every param-page
 * render, so overriding it lets us format the time fresh on each page load.
 */
class CurrentTimeText : public WiFiManagerParameter {
public:
    CurrentTimeText() : WiFiManagerParameter("") {}

    const char* getCustomHTML() const override
    {
        uint32_t epoch = Clock::getInstance().now();

        if (epoch == 0)
        {
            mHtml = "<label>Device time</label><br/>not synced";
            return mHtml.c_str();
        }

        time_t t = static_cast<time_t>(epoch);
        char when[24] = {0};
        strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", gmtime(&t));

        char buffer[128] = {0};
        snprintf(buffer, sizeof(buffer),
                 "<label>Device time (UTC)</label><br/>"
                 "<b>%s</b><br/>Epoch: %u (synced)",
                 when, epoch);

        mHtml = buffer;
        return mHtml.c_str();
    }

private:
    mutable std::string mHtml;
};

}  // namespace app
