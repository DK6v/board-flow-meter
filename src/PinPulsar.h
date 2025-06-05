#pragma once

#include <Arduino.h>

#include "TimerDispatcher.h"
#include "Reporter.h"
#include "PinBase.h"
#include "PinOut.h"

#include "Pulsar.h"

namespace app {

// -------------------------------------------------------

class PinPulsar : public TimerListener {
public:
    PinPulsar(Reporter& reporter, PinOut& power);
    ~PinPulsar() = default;

    operator Pulsar() { return mPulsar; }

    // Inplement TimerListener
    void onTimer();

    void sendMetric();
    
private:
    Reporter& mReporter;
    PinOut& mPower;

#if defined(ESP8266)
    SoftwareSerial mSerial;
#endif
    Pulsar mPulsar;

    float mLastHeatEnergy;
    secs mLastReportTime;
};

} // namespace fm
