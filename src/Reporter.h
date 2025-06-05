#pragma once

#include <list>
#include <array>
#include <string>
#include <sstream>

#include <Arduino.h>
#include "OneWire.h"
#include "DallasTemperature.h"

#include <WiFiManager.h>

namespace app {

class Reporter {   
public:
    Reporter(const char* host, uint16_t port);
    Reporter(const std::string& host, uint16_t port);
    ~Reporter() = default;

    size_t send(std::string metric);

private:
    enum Status {
        CONNECTED = 0,
        FAILED,
        UNKNOWN
    };
private:
    std::string mHost;
    uint16_t mPort;

    Status mStatus;
};

} // namespace fm