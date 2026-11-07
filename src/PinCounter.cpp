#include "Arduino.h"

#include "PinCounter.h"

using namespace reporter;

namespace app {

PinCounter::PinCounter(uint8_t pin,
                       Reporter &reporter,
                       const char *name,
                       const uint32_t multiplier = 1)
    : PinBase(pin),
      mName(name),
      mReporter(reporter),
      mBucket(0),
      mCounterTotal(0),
      mMultiplier(multiplier)
{
    pinMode(pin, INPUT);
    mLastState = digitalRead(mPin);
    mLastCheckMs = millis();
}

void PinCounter::attach(VoidCallbackPtr callback) const
{
    pinMode(mPin, INPUT);
    attachInterrupt(mPin, callback, ONLOW);
}

bool PinCounter::process() {

    pinMode(mPin, INPUT);

    msec currentTimeMs = millis();

    if (currentTimeMs - mLastCheckMs >= 1000) {

        mLastCheckMs = currentTimeMs;

        if ((mLastState == LOW) && (digitalRead(mPin) == HIGH)) {

            mLastState = HIGH;

            mBucket += mMultiplier;
            mCounterTotal += mMultiplier;

            return true;
        }
        else if ((mLastState == HIGH) && (digitalRead(mPin) == LOW)) {

            mLastState = LOW;
        }
    }

    return false;
}

PinCounter& PinCounter::operator++() {

    mBucket += mMultiplier;
    mCounterTotal += mMultiplier;

    return *this;
}

void PinCounter::onTimer()
{
    sendMetric();
}

bool PinCounter::sendMetric() {

    mReporter.clear();
    mReporter.addTag("device_type", "water_meter");
    mReporter.addTag("entity_id", std::string(mName));
    mReporter.addTag("entity_type", "counter");
    mReporter.addTag("entity_unit", "L");
    mReporter.addField("value", mCounterTotal);
    bool sent = mReporter.send() > 0;

    mReporter.clear();
    mReporter.addTag("device_type", "water_meter");
    mReporter.addTag("entity_id", std::string(mName));
    mReporter.addTag("entity_type", "delta");
    mReporter.addTag("entity_unit", "L");
    mReporter.addField("value", mBucket);
    sent = (mReporter.send() > 0) && sent;

    // The delta is kept until it's sent, so a failed connection doesn't lose it
    if (sent)
    {
        mBucket = 0;
    }

    return sent;
}

long PinCounter::getValue()
{
    return mCounterTotal;
}

void PinCounter::setValue(unsigned long value)
{
    mCounterTotal = value;
    mBucket = 0;
}

} // namespace app