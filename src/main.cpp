/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#undef LOG_MODULE
#define LOG_MODULE "MAIN"

#include <string>
#include <vector>

#include <Arduino.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

#include <console.h>
#include <checksum.h>
#include <reporter.h>
#include <config.h>
#include <console/SyslogReporter.h>

#include "TimerDispatcher.h"
#include "sensor.h"
#include "NonVolitileCounter.h"
#include "PinLed.h"
#include "PinPzem.h"
#include "PinPulsar.h"
#include "PinCounter.h"
#include "WmConfig.h"

#define TICK_MS (10)

#define GCAL(calorie) (static_cast<float>(calorie) / 1'000'000)
#define KWH(Wh) (static_cast<float>(Wh) / 1'000.0)
#define M3(liter) (static_cast<float>(liter) / 1'000.0)

// using namespace console;
using namespace reporter;
using namespace config;

using CONFIG = config::Config;

constexpr CONFIG::ID COLD_WATER = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 5);
constexpr CONFIG::ID HOT_WATER = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 6);
constexpr CONFIG::ID ELECTRICITY = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 7);
constexpr CONFIG::ID HEAT_ENERGY = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 8);

constexpr CONFIG::ID COLD_WATER_CORRECTION = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 11);
constexpr CONFIG::ID HOT_WATER_CORRECTION = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 12);
constexpr CONFIG::ID ELECTRICITY_CORRECTION = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 13);
constexpr CONFIG::ID HEAT_ENERGY_CORRECTION = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 14);

constexpr CONFIG::ID MEASUREMENT = static_cast<CONFIG::ID>(CONFIG::ID::CUSTOM_START + 21);

/*
 * D0
 * D1   RX_2
 * D2   TX_2
 * D3   red led             boot fails if pulled LOW
 * D4   dallas sendors      boot fails if pulled LOW
 * D5   counter (cold)
 * D6   counter (hot)
 * D7
 * D8                       fails if pulled HIGH
 */

std::string mac;
WiFiManager wm;

app::PinLed redLed(PIN_D3);
app::PinLed blueLed(PIN_D4);
app::PinOut pulsarPowerPin(PIN_D8);

app::TimerDispatcher td;
const auto & wifiClient = WiFiClient();
reporter::InfluxReporter defaultReporter(wifiClient, INFLUX_HOST, INFLUX_PORT);

app::PinCounter waterColdCounter(PIN_D5, defaultReporter, "cold", 10);
app::PinCounter waterHotCounter(PIN_D6, defaultReporter, "hot", 10);
app::PinPzem pzemPowerMeter(defaultReporter);
app::PinPulsar pulsarHeatMeter(defaultReporter, pulsarPowerPin);
app::DSSensorPin sensors(PIN_D4, defaultReporter);

app::StringParameter paramMeasurement("measurement", "Measurement", "");
app::FloatParameter paramColdWaterTotal("cold_water_total", "Cold Water Total (M3)", 0.0);
app::FloatParameter paramHotWaterTotal("hot_water_total", "Hot Water Total (M3)", 0.0);
app::FloatParameter paramElectricityTotal("electricity_total", "Electricity Total (kWh)", 0.0);
app::FloatParameter paramHeatEnergyTotal("heat_energy_total", "Heat Energy Total (Gcal)", 0.0);
app::FloatParameter paramPowerCorrection("power_correction", "Power Correction (Wh)", 0.0);

#include <string>
#include <cstdarg>
#include <cstdio>

template <size_t size = 32>
inline std::string format(const char *fmt, ...)
{
    char buffer[size];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return std::string(buffer);
}

void saveParamsCallback () {

    auto eeprom = config::StorageEeprom(1024);
    auto & config = config::getInstance();

    {
        std::string value = paramMeasurement.getValue();
        std::string persistent = config.getOr<std::string>(MEASUREMENT, "");
        if (value != persistent)
        {
            LOGI("measurement, set new value=%s, previous=%s", value.c_str(), persistent.c_str());
            config.set<std::string>(MEASUREMENT, value);

            defaultReporter.create(value.c_str());
        }
    }

    {
        auto value = static_cast<uint32_t>(paramColdWaterTotal.getValue() * 1'000);
        auto persistent = config.getOr<PersistCounter>(COLD_WATER, 0).get();
        if (value != persistent)
        {
            LOGI("cold water, set new value=%u, previous=%u", value, persistent);
            config.set<PersistCounter>(COLD_WATER, value);
            waterColdCounter.setValue(value);
        }
    }

    {
        auto value = static_cast<uint32_t>(paramHotWaterTotal.getValue() * 1'000);
        auto persistent = config.getOr<PersistCounter>(HOT_WATER, 0).get();
        if (value != persistent)
        {
            LOGI("hot water, set new value=%u, previous=%u", value, persistent);
            config.set<PersistCounter>(HOT_WATER, value);
            waterHotCounter.setValue(value);
        }
    }

    {
        auto value = static_cast<uint32_t>(paramElectricityTotal.getValue() * 1'000);
        auto persistent = config.getOr<PersistCounter>(ELECTRICITY, 0).get();
        if (value != persistent)
        {
            LOGI("electricity, set new value=%u, previous=%u", value, persistent);
            config.set<PersistCounter>(ELECTRICITY, value);
            pzemPowerMeter.setValue(value);
        }
    }

    {
        auto value = paramPowerCorrection.getValue();
        auto persistent = config.getOr<float>(ELECTRICITY_CORRECTION, 0.0);
        if (value != persistent)
        {
            LOGI("electricity correction, set new value=%g, previous=%g", value, persistent);
            config.set<float>(ELECTRICITY_CORRECTION, value);
            pzemPowerMeter.correction(value);
        }
    }

    config.write(eeprom);
}

void setup() {

#if LOG_LEVEL >= LOG_LEVEL_ERROR
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
#endif

    LOG("\n");
    LOG("cplusplus:%u", __cplusplus);
    LOG("=> start <=");

    WiFi.mode(WIFI_STA);

    redLed.setDimm(30);
    redLed.blink();

    sensors.search();

    pinMode(SETUP_PIN, INPUT_PULLUP);
    delay(1000);

    mac = WiFi.macAddress().c_str();
    std::erase_if(mac, [](char c) { return c == ':'; });

    std::string wifiApName = mac;
    std::ranges::transform(wifiApName, wifiApName.begin(), ::toupper);

    config::StorageEeprom eeprom = config::StorageEeprom(1024);

    auto & config = config::getInstance()
        .add<PersistCounter>(COLD_WATER, PersistCounter(0, 30))
        .add<PersistCounter>(HOT_WATER, PersistCounter(0, 30))
        .add<PersistCounter>(ELECTRICITY, PersistCounter(0, 30))
        .add<PersistCounter>(HEAT_ENERGY, PersistCounter(0, 30))
        .add<float>(ELECTRICITY_CORRECTION, 1.0)
        .add<std::string>(MEASUREMENT, "test")
        .read(eeprom)
        .write(eeprom);

    waterColdCounter.setValue(config.getOr<PersistCounter>(COLD_WATER, 0));
    waterHotCounter.setValue(config.getOr<PersistCounter>(HOT_WATER, 0));
    pzemPowerMeter.setValue(config.getOr<PersistCounter>(ELECTRICITY, 0));

    if (!wm.autoConnect(wifiApName.c_str()))
    {
        Serial.println("Wifi has been configured, restart");
        delay(1000);
        ESP.restart();
    }

    // Prapare configuration parameters

    {
        std::string value = config.getOr<std::string>(MEASUREMENT, "test");
        paramMeasurement.setValue(
            value.c_str(),
            paramMeasurement.getValueLength());

        defaultReporter.create(value.c_str());
        defaultReporter.addHeader("device_id", mac);
    }

    {
        float value = M3(config.getOr<PersistCounter>(COLD_WATER, 0));
        paramColdWaterTotal.setValue(
            format("%.3f", value).c_str(),
            paramColdWaterTotal.getValueLength());
    }

    {
        float value = M3(config.getOr<PersistCounter>(HOT_WATER, 0));
        paramHotWaterTotal.setValue(
            format("%.3f", value).c_str(),
            paramHotWaterTotal.getValueLength());
    }

    {
        float value = KWH(config.getOr<PersistCounter>(ELECTRICITY, 0));
        paramElectricityTotal.setValue(
            format("%.3f", value).c_str(),
            paramElectricityTotal.getValueLength());
    }

    {
        auto value = GCAL(config.getOr<PersistCounter>(HEAT_ENERGY, 0));
        paramHeatEnergyTotal.setValue(
            format("%.6f", value).c_str(),
            paramHeatEnergyTotal.getValueLength());
    }

    paramPowerCorrection.setValue(
        String(config.getOr<float>(ELECTRICITY_CORRECTION, 1.0)).c_str(),
        paramPowerCorrection.getValueLength());

    wm.addParameter(&paramMeasurement);
    wm.addParameter(&paramColdWaterTotal);
    wm.addParameter(&paramHotWaterTotal);
    wm.addParameter(&paramElectricityTotal);
    wm.addParameter(&paramHeatEnergyTotal);
    wm.addParameter(&paramPowerCorrection);
    wm.setSaveParamsCallback(saveParamsCallback);

    static auto sensorOnTimerCallback = []()
    {
        auto &config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());

        LOGD("dallas sensors reporter timer expired");
        sensors.onTimer();
    };

    static auto waterColdOnTimerCallback = []()
    {
        auto &config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("cold water reporter timer expired");
        waterColdCounter.onTimer();
    };

    static auto waterHotOnTimerCallback = []()
    {
        auto &config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("hot water reporter timer expired");
        waterHotCounter.onTimer();
    };

    static auto pzemOnTimerCallback = []()
    {
        auto & config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("power meter timer expired");
        pzemPowerMeter.onTimer();

        float value = pzemPowerMeter.getValue();
        paramElectricityTotal.setValue(
            format("%.3f", KWH(value)).c_str(),
            paramElectricityTotal.getValueLength());

        config.set<PersistCounter>(ELECTRICITY, value);
        auto eeprom = config::StorageEeprom(1024);
        config.write(eeprom);
    };

    static auto pulsarOnTimerCallback = []()
    {
        auto & config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("heat meter timer expired");
        pulsarHeatMeter.onTimer();

        float value = pulsarHeatMeter.getValue();
        paramHeatEnergyTotal.setValue(
            format("%.6f", GCAL(value)).c_str(),
            paramHeatEnergyTotal.getValueLength());

        config.set<PersistCounter>(HEAT_ENERGY, value);
        auto eeprom = config::StorageEeprom(1024);
        config.write(eeprom);
    };

    wm.setWebServerCallback([&]() {
        wm.server->on("/request", HTTP_GET, [&]() {
            wm.server->send(200, "text/plain", "Measurements update have been requested.");

            pulsarOnTimerCallback();
            pzemOnTimerCallback();
            sensorOnTimerCallback();
        });
    });

    std::vector<const char *> menu = {"wifi", "param", "info", "exit", "update"};
    wm.setMenu(menu);
    wm.startWebPortal();

    delay(1000);

#if defined(SYSLOG_HOST) && defined(SYSLOG_PORT)
    auto &syslog = console::SyslogReporter::getInstance();
    syslog.init(SYSLOG_HOST, SYSLOG_PORT, mac.c_str());
#endif

    td.startTimer(waterColdOnTimerCallback, (15 * app::MINUTES));
    td.startTimer(waterHotOnTimerCallback, (15 * app::MINUTES));
    td.startTimer(sensorOnTimerCallback, (15 * app::MINUTES));
    td.startTimer(pulsarOnTimerCallback, (30 * app::MINUTES));
    td.startTimer(pzemOnTimerCallback, (30 * app::MINUTES));

    redLed.blink();
    redLed.blink();

    LOG("=> setup is done <=");
    LOG("- config.measurement=%s", config.getOr<std::string>(MEASUREMENT, "").c_str());
    LOG("- config.cold_water=%u", config.getOr<PersistCounter>(COLD_WATER, 0));
    LOG("- config.hot_water=%u", config.getOr<PersistCounter>(HOT_WATER, 0));
    LOG("- config.electricity=%u", config.getOr<PersistCounter>(ELECTRICITY, 0));
    LOG("- config.heat_energy=%u", config.getOr<PersistCounter>(HEAT_ENERGY, 0));
    LOG("- config.electricity_correction=%.6f", config.getOr<float>(ELECTRICITY_CORRECTION, 1.0));

    LOG("Water counters (L): cold=%u, hot=%u",
        waterColdCounter.getValue(),
        waterHotCounter.getValue());
    LOG("Pulsar heat meter (cal): %u", pulsarHeatMeter.getValue());
    LOG("Pzem power meter (Wh): %u", pzemPowerMeter.getValue());
}

void loop() {

    if (waterColdCounter.process())
    {
        LOGD("process cold water counter");

        float value = waterColdCounter.getValue();
        paramColdWaterTotal.setValue(
            format("%.3f", M3(value)).c_str(),
            paramColdWaterTotal.getValueLength());

        auto &config = config::getInstance();
        config.set<PersistCounter>(COLD_WATER, value);
        auto eeprom = config::StorageEeprom(1024);
        config.write(eeprom);
    }

    if (waterHotCounter.process())
    {
        LOGD("process hot water counter");

        float value = waterHotCounter.getValue();
        paramHotWaterTotal.setValue(
            format("%.3f", M3(value)).c_str(),
            paramHotWaterTotal.getValueLength());

        auto &config = config::getInstance();
        config.set<PersistCounter>(HOT_WATER, value);
        auto eeprom = config::StorageEeprom(1024);
        config.write(eeprom);
    }

    wm.process();
    td.process();

    delay(TICK_MS);
}