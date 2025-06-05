#pragma once

#include <list>
#include <array>
#include <string>
#include <sstream>

#include <Arduino.h>
#include "OneWire.h"
#include "DallasTemperature.h"

#include <WiFiManager.h>

#include "TimerDispatcher.h"
#include "Reporter.h"
#include "NonVolitileCounter.h"

#include "PinBase.h"

namespace app {

class DSSensorPin : PinBase, public TimerListener {
public:
    class Sensor {
    public:
        Sensor() : address(), tempC(0.0) { address.fill(0); }
        ~Sensor() = default;

        operator uint8_t * () { return address.data(); };
        operator const std::string () const {
            std::stringstream ss;
            for (auto it = address.rbegin(); it != address.rend(); ++it) {
                ss << std::hex << std::uppercase << (int)*it;
            }
            return ss.str();
        }
        
        void addParameters(WiFiManager& wm);

    public:
        std::array<uint8_t, 8U> address;
        float tempC;
    private:
        uint16_t port;

        std::string mParamHeader;
    };
    using SensorIterator = std::list<Sensor>::iterator;

public:
    DSSensorPin(uint8_t pin, Reporter& reporter);
    ~DSSensorPin() = default;

    std::list<Sensor>::const_iterator begin() const;
    std::list<Sensor>::const_iterator end() const;

    void search();
    void read();

    app::Result get(std::string addr, float* out) const;

    void addParameters(WiFiManager& wm);

    void send(Reporter& reporter);

    // Implement TimerListener
    void onTimer() override;

private:
    OneWire mOneWire;
    Reporter& mReporter;

    std::list<Sensor> mSensors;
};

} // namespace fm