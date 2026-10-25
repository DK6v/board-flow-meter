#pragma once

#include <list>
#include <array>
#include <string>

#include <Arduino.h>
#include "OneWire.h"
#include "DallasTemperature.h"

#include <reporter.h>

#include "TimerDispatcher.h"

#include "PinBase.h"

namespace app {

class DSSensorPin : PinBase, public TimerListener {
public:
    class Sensor {
    public:
        Sensor() : mAddress(), tempC(0.0)
        {
            mAddress.fill(0);
        }

        ~Sensor() = default;

        operator uint8_t *() { return mAddress.data(); };
        operator const std::string () const { return address(); }

        std::string address() const;

    public:
        std::array<uint8_t, 8U> mAddress;
        float tempC;
    private:
        uint16_t port;
    };
    using SensorIterator = std::list<Sensor>::iterator;

public:
    DSSensorPin(uint8_t pin, reporter::Reporter& reporter);
    ~DSSensorPin() = default;

    std::list<Sensor>::const_iterator begin() const;
    std::list<Sensor>::const_iterator end() const;

    void search();
    void read();

    app::Result get(const std::string& addr, float* out) const;

    void send(reporter::Reporter& reporter);

    void onTimer() override;

private:
    OneWire mOneWire;
    reporter::Reporter& mReporter;

    std::list<Sensor> mSensors;
};

} // namespace app