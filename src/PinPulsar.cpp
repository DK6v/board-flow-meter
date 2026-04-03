/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iomanip>

#include <Arduino.h>
#include <macros.h>

#include "PinPulsar.h"

using namespace reporter;

namespace app {

PinPulsar::PinPulsar(Reporter &reporter, PinOut &power)
    : mReporter(reporter),
      mPower(power),
      mSerial(PIN_D1, PIN_D2),
      mPulsar(mSerial, 0x03574677),
      mHeatEnergyTotal(0)
{
    mSerial.begin(9600);
}

void PinPulsar::onTimer() {
    sendMetric();
}

void PinPulsar::sendMetric() {

    console::disable(); // syslog is still enabled

    mPower.on();
    delay(1 * app::SECONDS);

    mSerial.begin(9600);

    if (mPulsar.update()) {

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "temperature_in");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "C");
        mReporter.addField("value", ROUNDF(mPulsar.mChTempIn, 2));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "temperature_out");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "C");
        mReporter.addField("value", ROUNDF(mPulsar.mChTempOut, 2));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "temperature_delta");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "C");
        mReporter.addField("value", ROUNDF(mPulsar.mChTempDelta, 2));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "energy");
        mReporter.addTag("entity_type", "counter");
        mReporter.addTag("entity_unit", "Gcal");
        mReporter.addField("value", ROUNDF(mPulsar.mChHeatEnergy, 6));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "power");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "Gcal/h");
        mReporter.addField("value", ROUNDF(mPulsar.mChHeatPower, 3));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "volume");
        mReporter.addTag("entity_type", "counter");
        mReporter.addTag("entity_unit", "m3");
        mReporter.addField("value", ROUNDF(mPulsar.mChWaterCapacity, 6));
        mReporter.send();

        mReporter.clear();
        mReporter.addTag("device_type", "heat_meter");
        mReporter.addTag("entity_id", "flow_rate");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "m3/h");
        mReporter.addField("value", ROUNDF(mPulsar.mChWaterFlow, 3));
        mReporter.send();

        mHeatEnergyTotal = static_cast<uint32_t>(mPulsar.mChHeatEnergy * 1'000'000.0);
    }

    mPower.off();
    console::restore();
}

uint32_t PinPulsar::getValue()
{
    return mHeatEnergyTotal;
}

} // namespace app