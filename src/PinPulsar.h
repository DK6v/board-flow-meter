/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <Arduino.h>

#include <reporter.h>

#include "TimerDispatcher.h"
#include "PinBase.h"
#include "PinOut.h"

#include "Pulsar.h"

namespace app {

// -------------------------------------------------------

class PinPulsar : public TimerListener {
public:
    PinPulsar(reporter::Reporter& reporter, PinOut& power);
    ~PinPulsar() = default;

    operator Pulsar() { return mPulsar; }

    void onTimer();
    void sendMetric();

    uint32_t getValue();

private:
    reporter::Reporter& mReporter;
    PinOut& mPower;

    SoftwareSerial mSerial;
    Pulsar mPulsar;

    uint32_t mHeatEnergyTotal;
};

} // namespace fm
