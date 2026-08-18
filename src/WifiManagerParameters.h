#pragma once

#include <ctime>
#include <string>

#include <Arduino.h>
#include <config.h>

#include <WiFiManager.h>

#include "network/Clock.h"

namespace app {

template <size_t size = 16>
class WMStringParameter : public WiFiManagerParameter {
public:
    WMStringParameter(const char *id, const char *placeholder, std::string value, const uint8_t length = size)
        : WiFiManagerParameter("") {
        init(id, placeholder, value.c_str(), length, "", WFM_LABEL_BEFORE);
    }

    std::string getValue() {
        return std::string(WiFiManagerParameter::getValue());
    }
};

class WMFloatParameter : public WiFiManagerParameter {
public:
    WMFloatParameter(const char *id, const char *placeholder, float value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    float getValue() {
        return String(WiFiManagerParameter::getValue()).toFloat();
    }
};

class WMIntParameter : public WiFiManagerParameter {
public:
    WMIntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    long getValue() {
        return String(WiFiManagerParameter::getValue()).toInt();
    }
};

class WMU32Parameter : public WiFiManagerParameter {
public:
    WMU32Parameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
      : WiFiManagerParameter("")
    {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    uint32_t getValue()
    {
        return static_cast<uint32_t>(String(WiFiManagerParameter::getValue()).toDouble());
    }
};

class WMCustomText : public WiFiManagerParameter {
public:
    WMCustomText(const char * text) : WiFiManagerParameter(text) {}
    WMCustomText(const std::string & text) : WiFiManagerParameter(text.c_str()) {}
};

// Read-only horizontal rule to visually divide param-page sections.
class WMSeparatorParameter : public WiFiManagerParameter {
public:
    WMSeparatorParameter() : WiFiManagerParameter("<hr>") {}
};

/*
 * Read-only WiFiManager param-page element that displays the device's current
 * wall-clock time (fetched from the CoAP config server via Clock).
 *
 * It is constructed with no id (WiFiManagerParameter(const char*) sets _id ==
 * NULL), so WiFiManager renders it as raw custom HTML and never treats it as an
 * input or saves it. getCustomHTML() is virtual and called on every param-page
 * render, so overriding it lets us format the time fresh on each page load.
 */
class WMDeviceTimeParameter : public WiFiManagerParameter {
public:
    WMDeviceTimeParameter() : WiFiManagerParameter("") {}

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

        char buffer[150] = {0};
        snprintf(buffer, sizeof(buffer),
                 "<label><b>Device time (UTC)</b></label><br/> %s <br/>"
                 "<label><b>Epoch time (synced)</b></label><br/> %u <br/>",
                 when, epoch);

        mHtml = buffer;
        return mHtml.c_str();
    }

private:
    mutable std::string mHtml;
};

/*
 * Read-only WiFiManager param-page element that displays the firmware
 * version string.  Constructed with no id so WiFiManager treats it as
 * raw custom HTML and never persists it.  getCustomHTML() is overridden
 * to render the version on every page load.
 */
class WMVersionParameter : public WiFiManagerParameter {
public:
    WMVersionParameter() : WiFiManagerParameter("") {}

    const char* getCustomHTML() const override
    {
        mHtml = "<label><b>Firmware version</b></label><br/>" APP_VERSION "<br/>";
        return mHtml.c_str();
    }

private:
    mutable std::string mHtml;
};

} // namespace app
