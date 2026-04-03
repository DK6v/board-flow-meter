#include <Arduino.h>
#include <console.h>

#include "NonVolitileCounter.h"

using namespace console;

namespace app {

NonVolitileCounter::NonVolitileCounter(uint32_t address, uint8_t extraSlots) :
    mCounter(0),
    mCommitRequired(false),
    mNumSlots(extraSlots + 1),
    mCurrentSlotIndex(0),
    mStartByteAddr(address),
    mFirstSlotAddr(address + 1),
    mCurrentSlotAddr(address + 1) {
}

void NonVolitileCounter::init(uint32_t value) {

    if (EEPROM.read(mStartByteAddr) != BYTE_START) {

        mCounter = value;

        LOG("nvc: init counter, start=%u", mStartByteAddr);

        EEPROM.write(mStartByteAddr, static_cast<uint8_t>(BYTE_START));

        EEPROM.put(mCurrentSlotAddr, static_cast<uint32_t>(mCounter));
        EEPROM.write(mCurrentSlotAddr + sizeof(uint32_t), static_cast<uint8_t>(BYTE_STOP));

        EEPROM.commit();
    }
    else {

        for (mCurrentSlotIndex = 0; mCurrentSlotIndex < mNumSlots; ++mCurrentSlotIndex) {

            if (EEPROM.read(mCurrentSlotAddr + sizeof(uint32_t)) == BYTE_STOP) {
                break;
            }

            mCurrentSlotAddr += sizeof(uint32_t);
        }

        EEPROM.get(mCurrentSlotAddr, mCounter);

        LOG("nvc: start=0x%x, slot=%u, address=0x%x, value=%u",
            mStartByteAddr, mCurrentSlotIndex, mCurrentSlotAddr, mCounter);
    }
}

NonVolitileCounter & NonVolitileCounter::operator++() {

    ++mCounter;

    EEPROM.put(mCurrentSlotAddr, static_cast<uint32_t>(mCounter));
    EEPROM.write(mCurrentSlotAddr + sizeof(uint32_t), static_cast<uint8_t>(BYTE_STOP));

    mCommitRequired = true;

    return *this;
}

void NonVolitileCounter::operator=(const uint32_t& value) {

    mCounter = value;

    EEPROM.put(mCurrentSlotAddr, static_cast<uint32_t>(mCounter));
    EEPROM.write(mCurrentSlotAddr + sizeof(uint32_t), static_cast<uint8_t>(BYTE_STOP));

    EEPROM.commit();
}

NonVolitileCounter::operator uint32_t() {
    return mCounter;
}

void NonVolitileCounter::onTimer() {

    if (mCommitRequired == true) {

        if (mCurrentSlotIndex + 1 < mNumSlots) {

            mCurrentSlotAddr += sizeof(uint32_t);
            ++mCurrentSlotIndex;
        }
        else {

            mCurrentSlotAddr = mFirstSlotAddr;
            mCurrentSlotIndex = 0;
        }

        EEPROM.commit();
        mCommitRequired = false;
    }
}

NonVolitileCounter & operator+=(NonVolitileCounter & counter, const uint32_t & value) {

    counter = counter + value;
    return counter;
}

} // namespace app