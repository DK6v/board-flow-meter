#include <Arduino.h>
#include <iomanip>

#include "PinPulsar.h"

namespace app {

#if defined(ESP8266)
PinPulsar::PinPulsar(Reporter &reporter, PinOut &power)
    : mReporter(reporter),
      mPower(power),
      mSerial(PIN_D1, PIN_D2),
      mPulsar(mSerial, 0x03574677),
      mLastHeatEnergy(0.0),
      mLastReportTime(app::TIME_INVALID) {
    
    mSerial.begin(9600);
}
#else
PinPulsar::PinPulsar(Reporter &reporter, PinOut &power)
    : mReporter(reporter),
      mPower(power),
      mPulsar(Serial, 0x03574677),
      mLastHeatEnergy(0.0),
      mLastReportTime(app::TIME_INVALID) {}
#endif
void PinPulsar::onTimer() {
    sendMetric();
}

void PinPulsar::sendMetric() {

    mPower.on();
    delay(3 * app::SECONDS);

    secs currentTime = millis() / app::SECONDS;

    if (mPulsar.update()) {
    
        std::stringstream ss;
    
        ss << std::fixed
           << "heat-meter,sensor=pulsar"
           << " tempIn="        << std::setprecision(2) << mPulsar.mChTempIn
           << ",tempOut="       << std::setprecision(2) << mPulsar.mChTempOut
           << ",tempDelta="     << std::setprecision(3) << mPulsar.mChTempDelta
           << ",heatEnergy="    << std::setprecision(6) << mPulsar.mChHeatEnergy
           << ",heatPower="     << std::setprecision(6) << mPulsar.mChHeatPower
           << ",waterCapacity=" << std::setprecision(2) << mPulsar.mChWaterCapacity
           << ",waterFlow="     << std::setprecision(2) << mPulsar.mChWaterFlow;
     
        if ((mLastHeatEnergy != 0.0) && (mLastReportTime != app::TIME_INVALID)) {
        
            ss << ",delta="    << std::setprecision(6) << (mPulsar.mChHeatEnergy - mLastHeatEnergy)
               << ",duration=" << std::setprecision(3) << (currentTime - mLastReportTime);
        }


        mReporter.send(ss.str());
        
        mLastHeatEnergy = mPulsar.mChHeatEnergy;
        mLastReportTime = currentTime;
    }

    delay(1 * app::SECONDS);
    mPower.off();
}

} // namespace app 