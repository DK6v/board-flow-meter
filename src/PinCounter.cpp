#include "Arduino.h"

#include "PinCounter.h"

namespace app {

PinCounter::PinCounter(uint8_t pin,
                       NonVolitileCounter& counter,
                       Reporter& reporter,
                       const char* name,
                       const uint32_t multiplier = 1)
    : PinBase(pin),
      mName(name),
      mReporter(reporter),
      mBucket(0),
      mTotal(counter),
      mMultiplier(multiplier) {
    
    pinMode(pin, INPUT);

    mLastState = digitalRead(mPin);
    mLastCheckMs = millis();
}

void PinCounter::attach(VoidCallbackPtr callback) const {
    pinMode(mPin, INPUT);
    attachInterrupt(mPin, callback, ONLOW);
}

void PinCounter::process() {
    
    pinMode(mPin, INPUT);

    msec currentTimeMs = millis();

    if (mLastCheckMs < (currentTimeMs - 1000)) {

        if ((mLastState == LOW) && (digitalRead(mPin) == HIGH)) {

            mLastState = HIGH;

            mBucket += mMultiplier;
            mTotal += mMultiplier;
        }
        else if ((mLastState == HIGH) && (digitalRead(mPin) == LOW)) {

            mLastState = LOW;
        }

        mLastCheckMs = currentTimeMs;
    }
}

uint32_t PinCounter::total() const {
    return mTotal;
}

bool PinCounter::PinCounter::empty() const {
    return (mBucket == 0);
}

void PinCounter::reset() {
    mBucket = 0;
}

PinCounter::operator uint32_t() const {
    return mBucket;
}

PinCounter& PinCounter::operator++() {

    mBucket += mMultiplier;
    return *this;
}

PinCounter& PinCounter::operator--() {

    if (mBucket >= mMultiplier) {
        mBucket -= mMultiplier;
    } else {
        mBucket = 0;
    }

    return *this;
}

void PinCounter::onInterrupt() {
    
    msec currentTimeMs = millis();
    
    if (mLastCheckMs < (currentTimeMs - 20)) {
        mBucket += mMultiplier;
        mTotal += mMultiplier;
    }
    mLastCheckMs = currentTimeMs;
}

void PinCounter::sendMetric() {

    if (mBucket != 0) {

        std::string metric = "water,sensor=" + std::string(mName)
                           + " count=" + std::to_string(mBucket)
                           + ",total=" + std::to_string(mTotal);

        if (0 != mReporter.send(metric)) {
            mBucket = 0;
        }
    }
}

void PinCounter::onTimer() {

    sendMetric();
}

long PinCounter::getValue() {

    return mTotal;
}

void PinCounter::setValue(long value) {

    mTotal = value;
}

} // namespace fm