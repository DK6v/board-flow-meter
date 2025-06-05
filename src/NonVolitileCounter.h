#pragma once

#include <Arduino.h>
#include <EEPROM.h>

#include "TimerDispatcher.h"

namespace app {

class NonVolitileCounter : public TimerListener {
public:
    NonVolitileCounter(uint32_t offset, uint8_t extraSlots = 3);
    ~NonVolitileCounter() = default;

    void init(uint32_t value = 0);

    NonVolitileCounter& operator++();

    void operator=(const uint32_t& value);
    operator uint32_t();

    // Implement TimerListener
    void onTimer();

protected:

    const uint8_t BYTE_START = 0xB7;
    const uint8_t BYTE_STOP = 0xB7;

    uint32_t mCounter;
    bool mCommitRequired;

    uint8_t mNumSlots;
    uint8_t mCurrentSlotIndex;

    uint32_t mStartByteAddr;
    uint32_t mFirstSlotAddr;
    uint32_t mCurrentSlotAddr;
};

NonVolitileCounter & operator+=(NonVolitileCounter & counter, const uint32_t & value);

} // namespace app