/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <vector>

#include <Arduino.h>
#include <HardwareSerial.h>
#if defined(ESP8266)
#include <SoftwareSerial.h>
#endif

#include "PinBase.h"

namespace app {

// -------------------------------------------------------

class Pulsar {
public:
#if defined(ESP8266)
    Pulsar(SoftwareSerial& port, uint32_t address);
#endif
    Pulsar(HardwareSerial& port, uint32_t address);

    ~Pulsar() = default;

    static constexpr uint32_t baudrate = 9600;
    static constexpr uint32_t readTimeout = 3000;
    static constexpr uint32_t updateInterval = 1000;

    enum Function {
        FN_READ_CHANNEL   = 0x01,
        FN_READ_PARAMETER = 0x0A
    };

    enum Parameter {
        PM_ID       = 0x0000,
        PM_ADRESS   = 0x0001,
        PM_VERSION  = 0x0002
    };

    enum Channel {
        CH_TEMPC_IN       = 3,
        CH_TEMPC_OUT      = 4,
        CH_TEMPC_DELTA    = 5,
        CH_HEAT_POWER     = 6,
        CH_HEAT_ENERGY    = 7,
        CH_WATER_CAPACITY = 8,
        CH_WATER_FLOW     = 9
    };

    enum Endianness { BE = 0, LE = 1 };

public:
    bool update();

public:
    bool read(Parameter pm);
    bool read(std::vector<Channel> ch);

protected:

    enum PacketOffset {
        IX_ADDRESS  = 0,
        IX_FUNCTION = 4,
        IX_SIZE     = 5,
        IX_DATA     = 6
    };

    template<typename T>
    uint8_t write(Function fn, T data);

    uint8_t write(uint8_t *buffer_p, uint8_t value);
    uint8_t write(uint8_t *buffer_p, uint16_t value, Endianness byteOrder);
    uint8_t write(uint8_t *buffer_p, uint32_t value, Endianness byteOrder);

    uint8_t receive(uint8_t *buffer, uint8_t length);

    float fetch(uint8_t *buffer_p, uint8_t index);

    bool checkCRC(const uint8_t *buffer_p, uint8_t length);
    uint16_t CRC16(const uint8_t *start_p, const uint8_t *end_p) const;

private:
    Stream* mSerial_p;
    bool mIsSoftSerial;

    uint32_t mAddress;
    uint16_t mUserData;

    uint32_t mLastUpdate;

public:
    /* Channel readings */
    float mChTempIn;
    float mChTempOut;
    float mChTempDelta;
    float mChHeatPower;
    float mChHeatEnergy;
    float mChWaterCapacity;
    float mChWaterFlow;
};

} // namespace app
