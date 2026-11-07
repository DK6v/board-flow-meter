/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#undef LOG_MODULE
#define LOG_MODULE "COAP"

#include "WifiTransport.h"

#include <console.h>

namespace app {

WifiTransport::WifiTransport()
{
    udp_.begin(0);
}

WifiTransport::~WifiTransport()
{
    udp_.stop();
}

bool WifiTransport::resolve(const std::string& host, std::string& address)
{
    IPAddress ip;
    if (!WiFi.hostByName(host.c_str(), ip, 2000))
    {
        LOGD("Resolving %s failed", host.c_str());
        return false;
    }

    address = ip.toString().c_str();
    return true;
}

bool WifiTransport::send(const std::string& address, uint16_t port,
                         const uint8_t* data, size_t size)
{
    IPAddress ip;
    if (!ip.fromString(address.c_str()))
    {
        return false;
    }

    if (!udp_.beginPacket(ip, port))
    {
        LOGD("Begin packet failed");
        return false;
    }

    udp_.write(data, size);
    return udp_.endPacket() == 1;
}

size_t WifiTransport::receive(uint8_t* buffer, size_t capacity,
                              std::string& fromAddress, uint16_t& fromPort,
                              uint32_t timeoutMs)
{
    unsigned long start = millis();

    while (millis() - start < timeoutMs)
    {
        int packetSize = udp_.parsePacket();
        if (packetSize > 0)
        {
            fromAddress = udp_.remoteIP().toString().c_str();
            fromPort = udp_.remotePort();

            size_t n = udp_.read(buffer, capacity);
            udp_.flush();
            return n;
        }

        delay(10);
    }

    return 0;
}

}  // namespace app
