#include <Arduino.h>
#include <iomanip>

#include "PinPzem.h"

namespace app {

#ifdef PZEM004_SOFTSERIAL
PinPzem::PinPzem(Reporter& reporter, float energyBase)
    : mReporter(reporter),
      mSerial(PIN_D7, PIN_D2),
      mPzem(mSerial),
      mEnergyBase(energyBase),
      mEnergySensor(0.0),
      mLastReportTime(app::TIME_INVALID) {

    mSerial.begin(9600);
}

#else
PinPzem::PinPzem(Reporter& reporter, float energyBase)
    : mReporter(reporter),
      mPzem(Serial, PIN_D7, PIN_D2),
      mEnergyBase(energyBase),
      mEnergySensor(0.0),
      mLastReportTime(app::TIME_INVALID) {}
#endif

void PinPzem::init(float value) {
    mEnergyBase = value;
}

void PinPzem::onTimer() {
    sendMetric();
}

void PinPzem::sendMetric() {

    secs currentTime = millis() / app::SECONDS;

    float voltage = mPzem.voltage();
    float current = mPzem.current();
    float power = mPzem.power();
    float energy = mPzem.energy();
    float frequency = mPzem.frequency();

    if (!std::isnan(voltage) &&
        !std::isnan(current) &&
        !std::isnan(power) &&
        !std::isnan(energy) &&
        !std::isnan(frequency)) {

        std::stringstream ss;

        ss << std::fixed
           << "watt-meter,sensor=pzem"
           << " voltage="   << std::setprecision(1) << voltage
           << ",current="   << std::setprecision(3) << current
           << ",frequency=" << std::setprecision(1) << frequency
           << ",power="     << std::setprecision(2) << power
           << ",energy="    << std::setprecision(3) << (mEnergyBase + energy);
 
        if ((mEnergySensor != 0.0) && (mLastReportTime != app::TIME_INVALID)) {
        
            ss << ",delta="    << std::setprecision(3) << (energy - mEnergySensor)
               << ",duration=" << std::setprecision(3) << (currentTime - mLastReportTime);
        }
 
        mReporter.send(ss.str());
    }

    mEnergySensor = energy;
    mLastReportTime = currentTime;
}

float PinPzem::getValue() {
    return mEnergyBase;
}

void PinPzem::setValue(float value) {

    mEnergyBase = value;
    mPzem.resetEnergy();
}

} // namespace app 