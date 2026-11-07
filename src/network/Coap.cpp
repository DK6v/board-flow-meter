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

// 2024-01-01 00:00:00 UTC
static constexpr uint32_t MIN_EPOCH = 1704067200;

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

    // The request is sent without a token and the response must repeat its message ID
    if (token_length != 0)
    {
        LOGD("Unexpected token length: %d", token_length);
        return std::nullopt;
    }

    if (result.message_id != messageId_)
    {
        LOGD("Unexpected message ID: %d, expected %d", result.message_id, messageId_);
        return std::nullopt;
    }

    // Code 2.xx is required
    if ((result.response_code & 0xE0) != 0x40)
    {
        LOGD("Unexpected code: 0x%02x", result.response_code);
        return std::nullopt;
    }

    // Skip token
    size_t pos = 4 + token_length;

    // Skip options, the delta and the length may be extended (13, 14)
    while (pos < packet.size() && packet[pos] != 0xFF)
    {
        uint8_t option_byte = packet[pos++];
        uint8_t delta = option_byte >> 4;
        uint16_t option_length = option_byte & 0x0F;

        if (delta == 15 || option_length == 15)
        {
            LOGD("Invalid option header: 0x%02x", option_byte);
            return std::nullopt;
        }

        if (delta == 13)
        {
            pos += 1;
        }
        else if (delta == 14)
        {
            pos += 2;
        }

        if (option_length == 13)
        {
            if (pos >= packet.size())
            {
                return std::nullopt;
            }
            option_length = packet[pos++] + 13;
        }
        else if (option_length == 14)
        {
            if (pos + 1 >= packet.size())
            {
                return std::nullopt;
            }
            option_length = ((packet[pos] << 8) | packet[pos + 1]) + 269;
            pos += 2;
        }

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

    if (!isPayloadValid)
    {
        return std::nullopt;
    }

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

    messageId_ = clientId;

    // Header
    coapPacket[packetSize++] = 0x40;                       // Ver=1, Type=CON, TKL=0
    coapPacket[packetSize++] = 0x01;                       // GET
    coapPacket[packetSize++] = (clientId >> 8) & 0xFF;     // Message ID (high)
    coapPacket[packetSize++] = clientId & 0xFF;            // Message ID (low)

    // First Uri-Path option
    coapPacket[packetSize++] = 0xB6;  // Delta=11, Length=6
    memcpy(&coapPacket[packetSize], "device", 6);
    packetSize += 6;

    // Longer options need the extended length, the resource is a MAC address
    if (resource.length() > 12)
    {
        LOGE("Resource is too long: %u", resource.length());
        return;
    }

    // Second Uri-Path option (continuing same option number)
    coapPacket[packetSize++] = resource.length();  // Delta=0, Length=<length>
    memcpy(&coapPacket[packetSize], resource.c_str(), resource.length());
    packetSize += resource.length();

    // The response is accepted only from the address the request is sent to
    if (!WiFi.hostByName(server_.c_str(), serverIp_, 2000))
    {
        LOGD("Resolving %s failed", server_.c_str());
        return;
    }

    if (udp_.beginPacket(serverIp_, port_))
    {
        udp_.write(coapPacket, packetSize);
        udp_.endPacket();
    }
    else
    {
        LOGD("Begin packet failed");
        return;
    }
    requestSent_ = true;

    LOGD("CoAP request sent (ix: %d, %d bytes)", ++ix, packetSize);
}

void Coap::listen()
{
    int packetSize = 0;

    if (!requestSent_)
    {
        LOGD("Listen: no request sent");
        udp_.stop();
        return;
    }

    for (uint8_t attempts = 100; attempts--; delay(10))
    {
        packetSize = udp_.parsePacket();
        if (packetSize > 0)
        {
            if ((udp_.remoteIP() == serverIp_) && (udp_.remotePort() == port_))
            {
                break;
            }

            LOGD("Skip packet from %s:%d", udp_.remoteIP().toString().c_str(), udp_.remotePort());
            udp_.flush();
            packetSize = 0;
        }
    }

    if (packetSize > 0)
    {
        LOGD("From: %s:%d", udp_.remoteIP().toString().c_str(), udp_.remotePort());
        LOGD("Packet size: %u", packetSize);

        std::vector<uint8_t> packet(packetSize);
        udp_.read(packet.data(), packet.size());

        data = parsePacket(packet).value_or(CoAPResult());

        // A time before 2024 means a corrupted or fake payload
        if (data.timestamp.has_value() && (*data.timestamp < MIN_EPOCH))
        {
            LOGW("Timestamp %u is out of range, ignored", *data.timestamp);
            data.timestamp.reset();
        }
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
