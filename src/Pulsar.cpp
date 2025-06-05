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

#include <Arduino.h>
#include "Pulsar.h"

namespace app {

#if defined(ESP8266)
Pulsar::Pulsar(SoftwareSerial& port, uint32_t address)
    : mSerial_p(nullptr),
      mIsSoftSerial(true),
      mAddress(address),
      mUserData(0xE679),
      mLastUpdate(0) {
    
    port.begin(Pulsar::baudrate);
    mSerial_p = static_cast<Stream*>(&port);
}
#endif

Pulsar::Pulsar(HardwareSerial& port, uint32_t address)
    : mSerial_p(nullptr),
      mIsSoftSerial(false),
      mAddress(address),
      mUserData(0xE679),
      mLastUpdate(0) {
    
    port.begin(Pulsar::baudrate);
    mSerial_p = static_cast<Stream*>(&port);
}

bool Pulsar::update() {

    if(mLastUpdate + updateInterval > millis()){
        return true;
    }

    return read(std::vector<app::Pulsar::Channel> {
                app::Pulsar::CH_TEMPC_IN,
                app::Pulsar::CH_TEMPC_OUT,
                app::Pulsar::CH_TEMPC_DELTA,
                app::Pulsar::CH_HEAT_ENERGY,
                app::Pulsar::CH_HEAT_POWER,
                app::Pulsar::CH_WATER_CAPACITY,
                app::Pulsar::CH_WATER_FLOW });
}

template<typename T>
uint8_t Pulsar::write(Function fn, T data) {

    uint8_t buffer[14];
    uint8_t* npos = buffer;

    npos += write(npos, static_cast<uint32_t>(mAddress), BE);                // address   (4)
    npos += write(npos, static_cast<uint8_t>(fn)); // parameter (1)
    npos += write(npos, static_cast<uint8_t>(10 + sizeof(data)));            // length    (1)
    npos += write(npos, data);                                               // data      (*)
    npos += write(npos, static_cast<uint16_t>(mUserData));                   // user data (2)
    npos += write(npos, static_cast<uint16_t>(CRC16(buffer, npos)));         // CRC16     (2) 

    Serial.print("sent: ");
    print(buffer, npos - buffer);

    return mSerial_p->write(buffer, (npos - buffer));
}

bool Pulsar::read(Parameter pm) {

    return write(FN_READ_PARAMETER, static_cast<uint16_t>(pm));
}

bool Pulsar::read(std::vector<Channel> ch) {

    uint32_t mask = 0;
    uint8_t buffer[64];

    for (auto &shift : ch) {
        mask |= 1 << (shift - 1);
    }

    write(FN_READ_CHANNEL, static_cast<uint32_t>(mask));

    uint8_t size = receive(buffer, sizeof(buffer));

    Serial.print("rcvd: ");
    print(buffer, size);

    if (size != 0) {

        uint8_t chIx = 0;
        std::sort(ch.begin(), ch.end());

        for (auto &channel : ch) {

            switch (channel) {

                case CH_TEMPC_IN:       { mChTempIn = fetch(buffer, chIx++); break; }
                case CH_TEMPC_OUT:      { mChTempOut = fetch(buffer, chIx++); break; }
                case CH_TEMPC_DELTA:    { mChTempDelta = fetch(buffer, chIx++); break; }
                case CH_HEAT_POWER:     { mChHeatPower = fetch(buffer, chIx++); break; }
                case CH_HEAT_ENERGY:    { mChHeatEnergy = fetch(buffer, chIx++); break; }
                case CH_WATER_CAPACITY: { mChWaterCapacity = fetch(buffer, chIx++); break; }
                case CH_WATER_FLOW:     { mChWaterFlow = fetch(buffer, chIx++); break; }

                default: {
                    chIx++;
                    break;
                }
            }
        }

        return true;
    }
    
    return false;
}

uint8_t Pulsar::receive(uint8_t *buffer, uint8_t length) {

    uint8_t index = 0;

#if defined(ESP8266)
    if(mIsSoftSerial) {
        ((SoftwareSerial*)mSerial_p)->listen();
    }
#endif
    unsigned long startTime = millis();

    while((index < length) && ((millis() - startTime) < readTimeout)) {

        if(mSerial_p->available() > 0) {

            buffer[index++] = (uint8_t)mSerial_p->read();
        }

        yield();
    }

    return checkCRC(buffer, index) ? index : 0; 
}

float Pulsar::fetch(uint8_t *buffer_p, uint8_t index = 0) {

    uint8_t *npos = &buffer_p[IX_DATA] + sizeof(uint32_t) * index;
    
    Serial.print("read: ");
    print(npos, 4);

    uint32_t dcba = (((0xFF & static_cast<uint32_t>(npos[0]))) |
                     ((0xFF & static_cast<uint32_t>(npos[1])) << 8) |
                     ((0xFF & static_cast<uint32_t>(npos[2])) << 16) |
                     ((0xFF & static_cast<uint32_t>(npos[3])) << 24));

    float ret = 0.0;
    memcpy(&ret, &dcba, sizeof(float));

    Serial.print("      ");
    Serial.println(ret);

    return ret;
}

uint8_t Pulsar::write(uint8_t *buffer_p, uint8_t value) {
    
    uint8_t* npos = buffer_p;

    *(npos++) = (0xFF & (value));

    return static_cast<uint8_t>(npos - buffer_p);
}

uint8_t Pulsar::write(uint8_t *buffer_p, uint16_t value,
                         Endianness byteOrder = LE) {
    
    uint8_t* npos = buffer_p;

    if(byteOrder == LE) {
        *(npos++) = (0xFF & (value));
        *(npos++) = (0xFF & (value >> 8));
    }
    else {
        *(npos++) = (0xFF & (value >> 8));
        *(npos++) = (0xFF & (value));
    }
    
    return static_cast<uint8_t>(npos - buffer_p);
}

uint8_t Pulsar::write(uint8_t *buffer_p, uint32_t value,
                         Endianness byteOrder = LE) {
    
    uint8_t* npos = buffer_p;

    if(byteOrder == LE) {
        *(npos++) = (0xFF & (value));
        *(npos++) = (0xFF & (value >> 8));
        *(npos++) = (0xFF & (value >> 16));
        *(npos++) = (0xFF & (value >> 24));
    }
    else {       
        *(npos++) = (0xFF & (value >> 24));
        *(npos++) = (0xFF & (value >> 16));
        *(npos++) = (0xFF & (value >> 8));
        *(npos++) = (0xFF & (value));
    }

    return static_cast<uint8_t>(npos - buffer_p);
}

bool Pulsar::checkCRC(const uint8_t *buffer_p, uint8_t length) {

    if(length > 2) {

        uint16_t crc = CRC16(buffer_p, buffer_p + length - 2);

        return ((buffer_p[length - 2] == (0xFF & crc)) &&
                (buffer_p[length - 1] == (0xFF & (crc >> 8))));
    }

    return false;
}


uint16_t Pulsar::CRC16(const uint8_t *start_p, const uint8_t *end_p) const {

    const uint8_t *npos = start_p;
    uint16_t res = 0xFFFF;

    while (npos != end_p)
    {
        res ^= static_cast<uint16_t>(*npos++);

        for (uint8_t shift = 0; shift < 8; ++shift) {

            if (res & 0x01) {
                res = (res >> 1) ^ 0xA001;
            }
            else {
                res >>= 1;
            }
        }
    }

    return res;
}

void Pulsar::print(uint8_t *buffer_p, uint8_t size) {

    for(uint16_t ix = 0; ix < size; ++ix) {
        char temp[6];
        sprintf(temp, "%.2x ", buffer_p[ix]);
        Serial.print(temp);
    }
    Serial.print((size == 0) ? "-- NOK" : " -- OK");
    Serial.println();
}

} // namespace app 