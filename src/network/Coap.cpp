#undef LOG_MODULE
#define LOG_MODULE "COAP"

#include "Coap.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <console.h>

#include "Cbor.h"

using namespace std;
using namespace app;

Coap::Coap(std::string server = "localhost", int port = 5683) : server_(server), port_(port)
{
    udp_.begin(port_);
}

Coap::~Coap()
{
    udp_.stop();
}

std::optional<size_t> Coap::parseHeader(const std::vector<uint8_t>& packet,
                                        Coap::CoAPResult& result)
{
    if (packet.size() < 4)
    {
        return std::nullopt;
    }

    // Parse CoAP header
    uint8_t token_length = packet[0] & 0x0F;
    result.response_code = packet[1];
    result.message_id = (packet[2] << 8) | packet[3];

    LOGD("Token length: %d, Response code: 0x%02x, Message ID: %d",
         token_length,
         result.response_code,
         result.message_id);

    // Code 2.xx is required
    if ((result.response_code & 0xE0) != 0x40)
    {
        LOGD("Unexpected code: 0x%02x", result.response_code);
        return std::nullopt;
    }

    // Skip token
    size_t pos = 4 + token_length;

    // Skip options
    while (pos < packet.size() && packet[pos] != 0xFF)
    {
        uint8_t option_byte = packet[pos++];
        uint8_t option_length = option_byte & 0x0F;
        pos += option_length;
    }

    if (pos >= packet.size() || packet[pos] != 0xFF)
    {
        LOGD("No payload marker found");
        return std::nullopt;
    }

    return pos + 1;
}

std::optional<Coap::CoAPResult> Coap::parsePacket(const std::vector<uint8_t>& packet)
{
    CoAPResult result;

    auto offset_opt = parseHeader(packet, result);
    if (!offset_opt)
    {
        LOGE("Failed to parse header");
        return std::nullopt;
    }

    Cbor payload(packet, *offset_opt);

    if (!payload.isType(Cbor::Type::MAP))
    {
        return std::nullopt;
    }

    bool isPayloadValid = true;
    auto remainingBytes = payload.parseMap().value_or(0);

    while (isPayloadValid && remainingBytes-- && !payload.isEmpty())
    {
        isPayloadValid = false;

        switch (payload.parseParameterID().value_or(0x00))
        {
            case 0x01:  // Timestamp (UTC)
            {
                if (auto value_opt = payload.parseNumber())
                {
                    isPayloadValid = true;
                    result.timestamp = static_cast<uint32_t>(*value_opt);
                    LOGD(" - timestamp: %u", *result.timestamp);
                }
            }
            break;

            case 0x02:  // Report interval (secs)
            {
                if (auto value_opt = payload.parseNumber())
                {
                    isPayloadValid = true;
                    result.reportInterval = static_cast<uint32_t>(*value_opt);
                    LOGD(" - report_interval: %u", *result.reportInterval);
                }
            }
            break;

            case 0x03:  // Skip empty reports
            {
                if (auto value_opt = payload.parseNumber())
                {
                    isPayloadValid = true;
                    result.skipEmptyReports = static_cast<uint8_t>(*value_opt) ? true : false;
                    LOGD(" - skip_empty_reports: %u", *result.skipEmptyReports);
                }
            }
            break;

            case 0x11:  // sensor A
            case 0x12:
            {
                if (auto value_opt = payload.parseNumber())
                {
                    isPayloadValid = true;
                    result.alfaSensor = static_cast<int16_t>(*value_opt);
                    LOGD(" - alfa_sensor: %d", *result.alfaSensor);
                }
            }
            break;

            case 0x13:  // sensor B
            {
                if (auto value_opt = payload.parseNumber())
                {
                    isPayloadValid = true;
                    result.betaSensor = static_cast<int16_t>(*value_opt);
                    LOGD(" - beta_sensor: %d", *result.betaSensor);
                }
            }
            break;

            default:
                LOGE("Unknown payload option");
                break;
        }
    }

    LOGD("Packet payload is %s", (isPayloadValid) ? "valid" : "invalid");

    return std::optional<CoAPResult>(std::move(result));
}

void Coap::request(std::string resource, uint16_t clientId)
{
    static uint16_t ix = 0;

    uint8_t coapPacket[128];
    int packetSize = 0;

    if (!clientId)
    {
        randomSeed(micros());
        clientId = random(0, 0xFFFF);
    }

    // Header
    coapPacket[packetSize++] = 0x40;                       // Ver=1, Type=CON, TKL=0
    coapPacket[packetSize++] = 0x01;                       // GET
    coapPacket[packetSize++] = (clientId >> 8) & 0xFF;     // Message ID (high)
    coapPacket[packetSize++] = clientId & 0xFF;            // Message ID (low)

    // First Uri-Path option
    coapPacket[packetSize++] = 0xB6;  // Delta=11, Length=6
    memcpy(&coapPacket[packetSize], "device", resource.length());
    packetSize += 6;

    // Second Uri-Path option (continuing same option number)
    coapPacket[packetSize++] = resource.length();  // Delta=0, Length=<length>
    memcpy(&coapPacket[packetSize], resource.c_str(), resource.length());
    packetSize += resource.length();

    if (udp_.beginPacket(server_.c_str(), port_))
    {
        udp_.write(coapPacket, packetSize);
        udp_.endPacket();
    }
    else
    {
        LOGD("Begin packet failed");
    }

    LOGD("CoAP request sent (ix: %d, %d bytes)", ++ix, packetSize);
}

void Coap::listen()
{
    int packetSize = 0;

    for (uint8_t attempts = 100; attempts--; delay(10))
    {
        packetSize = udp_.parsePacket();
        if (packetSize > 0)
        {
            break;
        }
    }

    if (packetSize > 0)
    {
        LOGD("From: %s:%d", udp_.remoteIP().toString().c_str(), udp_.remotePort());
        LOGD("Packet size: %u", packetSize);

        std::vector<uint8_t> packet(packetSize);
        udp_.read(packet.data(), packet.size());

        data = parsePacket(packet).value_or(CoAPResult());
    }
    else
    {
        LOGD("Listen: no response received");
    }

    udp_.stop();
}

const Coap& Coap::sync(std::string resource)
{
    request(resource);
    listen();

    return *this;
}
