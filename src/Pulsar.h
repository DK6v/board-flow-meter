/* Copyright (c) 2023 Dmitry Korobkov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
    static constexpr uint32_t readTimeout = 1000;
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

    void print(uint8_t *buffer_p, uint8_t size);

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
