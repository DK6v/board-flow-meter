#pragma once

#include <Arduino.h>
#include <config.h>

#include <WiFiManager.h>

namespace app {

template <size_t size = 16>
class StringParameter : public WiFiManagerParameter {
public:
    StringParameter(const char *id, const char *placeholder, std::string value, const uint8_t length = size)
        : WiFiManagerParameter("") {
        init(id, placeholder, value.c_str(), length, "", WFM_LABEL_BEFORE);
    }

    std::string getValue() {
        return std::string(WiFiManagerParameter::getValue());
    }
};

class FloatParameter : public WiFiManagerParameter {
public:
    FloatParameter(const char *id, const char *placeholder, float value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    float getValue() {
        return String(WiFiManagerParameter::getValue()).toFloat();
    }
};

class IntParameter : public WiFiManagerParameter {
public:
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    long getValue() {
        return String(WiFiManagerParameter::getValue()).toInt();
    }
};

class U32Parameter : public WiFiManagerParameter {
public:
    U32Parameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
      : WiFiManagerParameter("")
    {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    uint32_t getValue()
    {
        return static_cast<uint32_t>(String(WiFiManagerParameter::getValue()).toDouble());
    }
};

class CustomText : public WiFiManagerParameter {
public:
    CustomText(const char * text) : WiFiManagerParameter(text) {}
    CustomText(const std::string & text) : WiFiManagerParameter(text.c_str()) {}
};

} // namespace app