#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <WiFiUdp.h>

namespace app {

class Coap
{
public:
    Coap(std::string server, int port);
    virtual ~Coap();

    const Coap& sync(std::string resource);

    virtual void request(std::string resource, uint16_t clientId = 0);
    virtual void listen();

private:
    struct CoAPResult {
        uint8_t response_code = 0;
        uint16_t message_id = 0;
        std::optional<uint32_t> timestamp;
        std::optional<int16_t> reportInterval;
        std::optional<uint8_t> skipEmptyReports;
        std::optional<int16_t> alfaSensor;
        std::optional<int16_t> betaSensor;
    };

    std::optional<size_t> parseHeader(const std::vector<uint8_t>& packet, Coap::CoAPResult& result);
    std::optional<Coap::CoAPResult> parsePacket(const std::vector<uint8_t>& packet);

public:
    CoAPResult data;

private:
    std::string server_;
    int port_;

    WiFiUDP udp_;
};

}  // namespace app
