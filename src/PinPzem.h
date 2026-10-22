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
    // Set the total, energy accumulated in the PZEM is still counted on the next read
    void setValue(uint32_t value);
    // Drop the energy accumulated in the PZEM, it's included in a total entered by the user
    void discardPending();

    void correction(float correction);

private:
    SoftwareSerial mSerial;
    PZEM004Tv30 mPzem;

    reporter::Reporter &mReporter;
    double mEnergyTotal;

    // Energy (Wh) in the PZEM counter that is already added to the total,
    // non-zero only if resetting the counter failed
    double mEnergyCounted;

    float mCorrection;
};

} // namespace app
