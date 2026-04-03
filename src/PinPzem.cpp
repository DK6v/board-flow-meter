#undef LOG_MODULE
#define LOG_MODULE "PZEM"

#include <iomanip>
#include <Arduino.h>
#include <console.h>

#include "PinPzem.h"

using namespace reporter;
using namespace config;

namespace app {

PinPzem::PinPzem(Reporter& reporter)
    : mSerial(PIN_D7, PIN_D2),
      mPzem(mSerial),
      mReporter(reporter),
      mEnergyTotal(0.0),
      mCorrection(0.0)
{
    mSerial.begin(9600);
}

void PinPzem::onTimer()
{
    sendMetric();
}

void PinPzem::sendMetric() {

    double voltage = mPzem.voltage();
    double current = mPzem.current();
    double power = mPzem.power();
    double energy = mPzem.energy();
    double frequency = mPzem.frequency();

    if (!std::isnan(voltage))
    {
        mReporter.clear();
        mReporter.addTag("device_type", "power_meter");
        mReporter.addTag("entity_id", "voltage");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "V");
        mReporter.addField("value", ROUNDF(voltage, 2));
        mReporter.send();
    }

    if (!std::isnan(current))
    {
        mReporter.clear();
        mReporter.addTag("device_type", "power_meter");
        mReporter.addTag("entity_id", "current");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "A");
        mReporter.addField("value", ROUNDF(current, 2));
        mReporter.send();
    }

    if (!std::isnan(power))
    {
        mReporter.clear();
        mReporter.addTag("device_type", "power_meter");
        mReporter.addTag("entity_id", "power");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "W");
        mReporter.addField("value", ROUNDF(power, 2));
        mReporter.send();
    }

    if (!std::isnan(frequency))
    {
        mReporter.clear();
        mReporter.addTag("device_type", "power_meter");
        mReporter.addTag("entity_id", "frequency");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "Hz");
        mReporter.addField("value", ROUNDF(frequency, 2));
        mReporter.send();
    }

    if (!std::isnan(energy))
    {
        mPzem.resetEnergy();

        double energyDelta = energy * 1000;
        energyDelta += energyDelta * mCorrection;

        mEnergyTotal += energyDelta;

        mReporter.clear();
        mReporter.addTag("device_type", "power_meter");
        mReporter.addTag("entity_id", "energy");
        mReporter.addTag("entity_type", "counter");
        mReporter.addTag("entity_unit", "W");
        mReporter.addField("value", ROUNDF(mEnergyTotal, 2));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "power_meter");
        mReporter.addTag("entity_id", "energy");
        mReporter.addTag("entity_type", "delta");
        mReporter.addTag("entity_unit", "W");
        mReporter.addField("value", ROUNDF(energyDelta, 2));
        mReporter.send();
    }
}

uint32_t PinPzem::getValue()
{
    return static_cast<uint32_t>(mEnergyTotal);
}

void PinPzem::setValue(uint32_t value)
{
    mEnergyTotal = value;
    mPzem.resetEnergy();
}

void PinPzem::correction(float correction)
{
    mCorrection = correction;
}

} // namespace