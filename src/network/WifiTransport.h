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
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <coap.h>

namespace app {

// UDP socket for coap::Client on the ESP8266
class WifiTransport : public coap::Transport {
public:
    WifiTransport();
    ~WifiTransport() override;

    bool resolve(const std::string& host, std::string& address) override;
    bool send(const std::string& address, uint16_t port,
              const uint8_t* data, size_t size) override;
    size_t receive(uint8_t* buffer, size_t capacity,
                   std::string& fromAddress, uint16_t& fromPort,
                   uint32_t timeoutMs) override;

private:
    WiFiUDP udp_;
};

}  // namespace app
