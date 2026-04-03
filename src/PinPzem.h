#pragma once

#include <Arduino.h>
#include "PZEM004Tv30.h"

#if defined(ESP8266)
#include <SoftwareSerial.h>
#endif

#include <reporter.h>
#include <config.h>

#include "TimerDispatcher.h"
#include "PinBase.h"

namespace app {

// -------------------------------------------------------

class PinPzem : public TimerListener {
public:
    PinPzem(reporter::Reporter& reporter);
    ~PinPzem() = default;

    void onTimer();
    void sendMetric();

    uint32_t getValue();
    void setValue(uint32_t value);

    void correction(float correction);

private:
    SoftwareSerial mSerial;
    PZEM004Tv30 mPzem;

    reporter::Reporter &mReporter;
    double mEnergyTotal;

    float mCorrection;
};

} // namespace fm
