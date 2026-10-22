/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#undef LOG_MODULE
#define LOG_MODULE "PULSAR"

#include <Arduino.h>
#include "Pulsar.h"

#include <console.h>

namespace app {

Pulsar::Pulsar(SoftwareSerial& port, uint32_t address)
    : mSerial_p(nullptr),
      mIsSoftSerial(true),
      mAddress(address),
      mUserData(0xE679),
      mLastUpdate(0) {

    port.begin(Pulsar::baudrate);
    mSerial_p = static_cast<Stream*>(&port);
}

Pulsar::Pulsar(HardwareSerial& port, uint32_t address)
    : mSerial_p(nullptr),
      mIsSoftSerial(false),
      mAddress(address),
      mUserData(0xE679),
      mLastUpdate(0) {

     port.begin(Pulsar::baudrate);
     mSerial_p = static_cast<Stream*>(&port);
 }

bool Pulsar::update()
{
    unsigned long startTime = millis();

    if (startTime - mLastUpdate < updateInterval)
    {
        LOGI("data is up to date %u ms", updateInterval - (startTime - mLastUpdate));
        return true;
    }

    // Retry until the meter answers or the time to wake it up is over
    for (unsigned attempt = 1; millis() - startTime < retryWindow; ++attempt)
    {
        if (read(std::vector<app::Pulsar::Channel>{
                app::Pulsar::CH_TEMPC_IN,
                app::Pulsar::CH_TEMPC_OUT,
                app::Pulsar::CH_TEMPC_DELTA,
                app::Pulsar::CH_HEAT_ENERGY,
                app::Pulsar::CH_HEAT_POWER,
                app::Pulsar::CH_WATER_CAPACITY,
                app::Pulsar::CH_WATER_FLOW}))
        {
            mLastUpdate = millis();

            LOGI("read attempt %u succeeded, %lu ms after the first request",
                 attempt, mLastUpdate - startTime);
            return true;
        }

        LOGI("read attempt %u failed", attempt);
    }

    return false;
}

template<typename T>
uint8_t Pulsar::write(Function fn, T data)
{
    uint8_t buffer[64];
    uint8_t* npos = buffer;

    npos += write(npos, static_cast<uint32_t>(mAddress), BE);                // address   (4)
    npos += write(npos, static_cast<uint8_t>(fn));                           // parameter (1)
    npos += write(npos, static_cast<uint8_t>(10 + sizeof(data)));            // length    (1)
    npos += write(npos, data);                                               // data      (*)
    npos += write(npos, static_cast<uint16_t>(mUserData));                   // user data (2)
    npos += write(npos, static_cast<uint16_t>(CRC16(buffer, npos)));         // CRC16     (2)

    u_int8_t size = npos - buffer;

    LOGV_ADD("write: sz=%u, data=[", size);
    for (uint16_t ix = 0; ix < size; ++ix)
    {
        LOGV_ADD(" %.2x", buffer[ix]);
    }
    LOGV_ADD(" ]");
    LOGV_FLUSH();

    u_int8_t result = mSerial_p->write(buffer, size);
    LOGD("send: serial write size=%u", result);

    return result;
}

bool Pulsar::read(Parameter pm) {

    uint8_t buffer[64];

    write(FN_READ_PARAMETER, static_cast<uint16_t>(pm));

    uint8_t size = receive(buffer, sizeof(buffer));
    LOGD_ADD("received: buffer size=%u", size);

    return (size > 0) ? true : false;
}

bool Pulsar::read(std::vector<Channel> ch) {

    uint32_t mask = 0;
    uint8_t buffer[64];

    if (mIsSoftSerial)
    {
        ((SoftwareSerial *)mSerial_p)->listen();
    }

    for (auto &shift : ch)
    {
        mask |= 1 << (shift - 1);
    }

    // Drop leftovers, e.g. a late answer to the previous request
    while (mSerial_p->available())
    {
        mSerial_p->read();
    }

    write(FN_READ_CHANNEL,
          static_cast<uint32_t>(mask));

    uint8_t size = receive(buffer, sizeof(buffer));

    // Header, a float per channel, user data and CRC
    if ((size > 0) && (size != IX_DATA + sizeof(float) * ch.size() + 4))
    {
        LOGE("unexpected response size=%u for %u channels", size, ch.size());
        size = 0;
    }
    LOGV_ADD("received: buffer size=%u", size);

    for (u_int8_t ix = 0; ix < size; ++ix)
    {
        if ((ix % 16) == 0)
        {
            LOGV_FLUSH();
            LOGV_ADD("%3u: ", ix);
        }
        else if ((ix % 4) == 0)
        {
            LOGV_ADD(" ");
        }

        LOGV_ADD("%02x", buffer[ix]);
    }
    LOGV_FLUSH();

    if (size > 0) {

        uint8_t chIx = 0;
        std::sort(ch.begin(), ch.end());

        for (auto &channel : ch) {

            switch (channel) {

                case CH_TEMPC_IN:
                {
                    mChTempIn = fetch(buffer, chIx++);
                    LOGI("CH_TEMPC_IN=%.2f", mChTempIn);
                    break;
                }
                case CH_TEMPC_OUT:
                {
                    mChTempOut = fetch(buffer, chIx++);
                    LOGI("CH_TEMPC_OUT=%.2f", mChTempOut);
                    break;
                }
                case CH_TEMPC_DELTA:
                {
                    mChTempDelta = fetch(buffer, chIx++);
                    LOGI("CH_TEMPC_DELTA=%.2f", mChTempDelta);
                    break;
                }
                case CH_HEAT_POWER:
                {
                    mChHeatPower = fetch(buffer, chIx++);
                    LOGI("CH_HEAT_POWER=%.3f", mChHeatPower);
                    break;
                }
                case CH_HEAT_ENERGY:
                {
                    mChHeatEnergy = fetch(buffer, chIx++);
                    LOGI("CH_HEAT_ENERGY=%.6f", mChHeatEnergy);
                    break;
                }
                case CH_WATER_CAPACITY:
                {
                    mChWaterCapacity = fetch(buffer, chIx++);
                    LOGI("CH_WATER_CAPACITY=%.6f", mChWaterCapacity);
                    break;
                }
                case CH_WATER_FLOW:
                {
                    mChWaterFlow = fetch(buffer, chIx++);
                    LOGI("CH_WATER_FLOW=%.3f", mChWaterFlow);
                    break;
                }

                default: {
                    chIx++;
                    break;
                }
            }
        }

        LOGI("pulsar data received");
        return true;
    }

    LOGE("failed to fetch pulsar data");
    return false;
}

uint8_t Pulsar::receive(uint8_t *buffer, uint8_t size) {

    // A response starts with the meter address, bytes before it are noise
    const uint8_t address[] = {
        static_cast<uint8_t>(mAddress >> 24), static_cast<uint8_t>(mAddress >> 16),
        static_cast<uint8_t>(mAddress >> 8), static_cast<uint8_t>(mAddress)};

    uint8_t index = 0;
    uint8_t length = 0;  // frame length from the header, 0 until received
    unsigned long startTime = millis();

    while (millis() - startTime < responseTimeout)
    {
        if (mSerial_p->available() <= 0)
        {
            delay(1);
            continue;
        }

        uint8_t value = static_cast<uint8_t>(mSerial_p->read());

        if ((index < sizeof(address)) && (value != address[index]))
        {
            LOGD("receive: skip byte 0x%02x", value);
            index = 0;
            if (value != address[0])
            {
                continue;
            }
        }

        buffer[index++] = value;

        if (index == IX_DATA)
        {
            // Address, function, length, user data and CRC at least
            length = buffer[IX_SIZE];
            if ((length < IX_DATA + 4) || (length > size))
            {
                LOGD("receive: invalid length=%u", length);
                return 0;
            }
        }

        if ((length > 0) && (index == length))
        {
            bool valid = checkCRC(buffer, length);
            LOGD("receive: size=%u, checksum is valid=%s, %lu ms",
                 length, valid ? "yes" : "NO", millis() - startTime);
            return valid ? length : 0;
        }
    }

    LOGD("receive timeout, size=%u", index);
    return 0;
}

float Pulsar::fetch(uint8_t *buffer_p, uint8_t index = 0) {

    uint8_t *npos = &buffer_p[IX_DATA] + sizeof(uint32_t) * index;
    u_int8_t size = 4;

    LOGD_ADD("fetch: sz=%u, data=[", size);
    for (uint16_t ix = 0; ix < size; ++ix)
    {
        LOGI_ADD(" %.2x", npos[ix]);
    }
    LOGD_ADD(" ]");
    LOGD_FLUSH();

    uint32_t dcba = (((0xFF & static_cast<uint32_t>(npos[0]))) |
                     ((0xFF & static_cast<uint32_t>(npos[1])) << 8) |
                     ((0xFF & static_cast<uint32_t>(npos[2])) << 16) |
                     ((0xFF & static_cast<uint32_t>(npos[3])) << 24));

    float retval = 0.0;
    memcpy(&retval, &dcba, sizeof(float));

    LOGD("fetch retval=%f", retval);

    return retval;
}

uint8_t Pulsar::write(uint8_t *buffer_p, uint8_t value)
{
    uint8_t* npos = buffer_p;

    *(npos++) = (0xFF & (value));

    return static_cast<uint8_t>(npos - buffer_p);
}

uint8_t Pulsar::write(uint8_t *buffer_p, uint16_t value, Endianness byteOrder = LE)
{
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

uint8_t Pulsar::write(uint8_t *buffer_p, uint32_t value, Endianness byteOrder = LE)
{
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

bool Pulsar::checkCRC(const uint8_t *buffer_p, uint8_t length)
{
    if(length > 2) {

        uint16_t crc = CRC16(buffer_p, buffer_p + length - 2);

        return ((buffer_p[length - 2] == (0xFF & crc)) &&
                (buffer_p[length - 1] == (0xFF & (crc >> 8))));
    }

    return false;
}


uint16_t Pulsar::CRC16(const uint8_t *start_p, const uint8_t *end_p) const
{
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

} // namespace app 