#include <list>
#include <array>
#include <string>
#include <sstream>

#include <Arduino.h>
#include <WiFiClient.h>

#include "sensor.h"
#include "Reporter.h"

namespace app {

Reporter::Reporter(const std::string& host, uint16_t port) :
    mHost(host),
    mPort(port),
    mStatus(Status::UNKNOWN) {}

Reporter::Reporter(const char* host, uint16_t port) :
    mHost(host),
    mPort(port) {}

size_t Reporter::send(std::string metric) {

    WiFiClient client;
    size_t bytesSent = 0;

    if (0 != client.connect(mHost.c_str(), mPort)) {
        bytesSent += client.printf(metric.c_str());
        client.flush();
        client.stop();
    }

    return bytesSent;
}

} // namespace fm