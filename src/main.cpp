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

#include <ctime>

#include <Arduino.h>
#include <flash_hal.h>

#include <console.h>
#include <checksum.h>
#include <reporter.h>
#include <config.h>
#include <portal.h>
#include <console/SyslogReporter.h>

#include "TimerDispatcher.h"
#include "sensor.h"
#include "PinLed.h"
#include "PinPzem.h"
#include "PinPulsar.h"
#include "PinCounter.h"

#include "network/Coap.h"
#include "network/Clock.h"

#define TICK_MS (10)

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
portal::Portal web;

app::PinLed redLed(PIN_D3);
app::PinLed blueLed(PIN_D4);
app::PinOut pulsarPowerPin(PIN_D8);

app::TimerDispatcher td;
const auto & wifiClient = WiFiClient();
reporter::InfluxReporter defaultReporter(wifiClient, INFLUX_HOST, INFLUX_PORT);
// A poll sends its metrics in one connection, an unreachable server delays it once
constexpr unsigned long REPORT_TIMEOUT_MS = 2000;

// Config journal in the FS area, the firmware doesn't mount a filesystem
config::FlashEsp configFlash(FS_PHYS_ADDR, FS_PHYS_SIZE);
config::Journal configJournal(configFlash);

app::PinCounter waterColdCounter(PIN_D5, defaultReporter, "cold", 10);
app::PinCounter waterHotCounter(PIN_D6, defaultReporter, "hot", 10);
app::PinPzem pzemPowerMeter(defaultReporter);
app::PinPulsar pulsarHeatMeter(defaultReporter, pulsarPowerPin);
app::DSSensorPin sensors(PIN_D4, defaultReporter);

// Wall-clock time from the CoAP config server, there is no RTC
std::string deviceTime()
{
    uint32_t epoch = app::Clock::getInstance().now();
    if (epoch == 0)
        return "not synced";

    time_t t = static_cast<time_t>(epoch);
    char buffer[24] = {0};
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
    return buffer;
}

void syncClock() {

    LOGD("request timestamp from CoAP config server %s:%d", CONFIG_HOST, CONFIG_PORT);

    app::Coap coap(CONFIG_HOST, CONFIG_PORT);
    coap.sync(mac);

    if (coap.data.timestamp.has_value())
    {
        uint32_t timestamp = *coap.data.timestamp;
        app::Clock::getInstance().sync(timestamp);
        LOGI("clock synced from CoAP config server, epoch=%u", timestamp);
    }
    else
    {
        LOGW("no timestamp received from CoAP config server");
    }
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

    auto & config = config::getInstance()
        .add<PersistCounter>(COLD_WATER, 0)
        .add<PersistCounter>(HOT_WATER, 0)
        .add<PersistCounter>(ELECTRICITY, 0)
        .add<PersistCounter>(HEAT_ENERGY, 0)
        .add<float>(ELECTRICITY_CORRECTION, 1.0)
        .add<std::string>(MEASUREMENT, "test");

    if (!config.read(configJournal))
    {
        LOGW("no stored config, defaults are used");
    }
    config.write(configJournal);

    waterColdCounter.setValue(config.getOr<PersistCounter>(COLD_WATER, 0));
    waterHotCounter.setValue(config.getOr<PersistCounter>(HOT_WATER, 0));
    pzemPowerMeter.setValue(config.getOr<PersistCounter>(ELECTRICITY, 0));
    pzemPowerMeter.correction(config.getOr<float>(ELECTRICITY_CORRECTION, 1.0));

    static auto sensorOnTimerCallback = []()
    {
        auto &config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());

        LOGD("dallas sensors reporter timer expired");
        defaultReporter.beginBatch();
        sensors.onTimer();
        defaultReporter.commit();
    };

    static auto waterColdOnTimerCallback = []()
    {
        auto &config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("cold water reporter timer expired");
        defaultReporter.beginBatch();
        waterColdCounter.onTimer();
        defaultReporter.commit();
    };

    static auto waterHotOnTimerCallback = []()
    {
        auto &config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("hot water reporter timer expired");
        defaultReporter.beginBatch();
        waterHotCounter.onTimer();
        defaultReporter.commit();
    };

    static auto pzemOnTimerCallback = []()
    {
        auto & config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("power meter timer expired");
        defaultReporter.beginBatch();
        pzemPowerMeter.onTimer();
        defaultReporter.commit();

        config.set<PersistCounter>(ELECTRICITY, pzemPowerMeter.getValue());
        config.write(configJournal);
    };

    static auto pulsarOnTimerCallback = []()
    {
        auto & config = config::getInstance();

        defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
        defaultReporter.addHeader("device_id", mac);

        LOGD("heat meter timer expired");

        // Keep the stored total if the meter didn't answer
        defaultReporter.beginBatch();
        bool updated = pulsarHeatMeter.sendMetric();
        defaultReporter.commit();

        if (!updated)
        {
            LOGW("heat meter read failed");
            return;
        }

        config.set<PersistCounter>(HEAT_ENERGY, pulsarHeatMeter.getValue());
        config.write(configJournal);
    };

    // Pages read the values from config, a saved form stores the changed
    // ones, writes the journal and applies them to the sensors
    web.name(wifiApName)
        .version(APP_VERSION)
        .ports(80, 8080)
        .storage(configJournal);

    web.counter(COLD_WATER, "cold_water_total", "Cold water", "m³", 1'000, 3)
        .section("Meters")
        .publish()
        .editable([](const uint32_t &value) {
            LOGI("cold water, set new value=%u", value);
            waterColdCounter.setValue(value);
        });

    web.counter(HOT_WATER, "hot_water_total", "Hot water", "m³", 1'000, 3)
        .publish()
        .editable([](const uint32_t &value) {
            LOGI("hot water, set new value=%u", value);
            waterHotCounter.setValue(value);
        });

    web.counter(ELECTRICITY, "electricity_total", "Electricity", "kWh", 1'000, 3)
        .publish()
        .editable([](const uint32_t &value) {
            LOGI("electricity, set new value=%u", value);
            pzemPowerMeter.setValue(value);
            pzemPowerMeter.discardPending();
        });

    // Read from the meter, not editable
    web.counter(HEAT_ENERGY, "heat_energy_total", "Heat energy", "Gcal", 1'000'000, 6).publish();

    web.number(ELECTRICITY_CORRECTION, "power_correction", "Power correction", "Wh/kWh", 2)
        .section("Settings")
        .hide()
        .editable([](const float &value) {
            LOGI("electricity correction, set new value=%g", value);
            pzemPowerMeter.correction(value);
        });

    web.text(MEASUREMENT, "measurement", "Measurement", 16)
        .hide()
        .editable([](const std::string &value) {
            LOGI("measurement, set new value=%s", value.c_str());
            defaultReporter.create(value.c_str());
        });

    web.item("Device time (UTC)", deviceTime, portal::Access::Public);

    web.on("/request", [](ESP8266WebServer &server) {
        server.send(200, "text/plain", "Measurements update have been requested.");

        pulsarOnTimerCallback();
        pzemOnTimerCallback();
        sensorOnTimerCallback();
    });

    // Don't hang in the setup AP forever (e.g. router boots slower after a
    // power outage): give up after a while and restart to retry the connection.
    web.setupTimeout(180);

    if (!web.connect())
    {
        LOGW("WiFi is not connected, restart");
        delay(1000);
        ESP.restart();
    }

    defaultReporter.create(config.getOr<std::string>(MEASUREMENT, "test").c_str());
    defaultReporter.addHeader("device_id", mac);
    defaultReporter.setTimeout(REPORT_TIMEOUT_MS);

    web.begin();

    delay(1000);

#if defined(SYSLOG_HOST) && defined(SYSLOG_PORT)
    auto &syslog = console::SyslogReporter::getInstance();
    syslog.init(SYSLOG_HOST, SYSLOG_PORT, mac.c_str());
#endif

    // The reason of the last reset goes to syslog, e.g. a watchdog or an exception
    LOGW("version=%s, reset: %s", APP_VERSION, ESP.getResetInfo().c_str());

    // Anchor wall-clock time to the CoAP config server, then keep it fresh
    // (corrects drift and survives the ~49.7 day millis() rollover).
    syncClock();

    static auto clockSyncCallback = []() {
        LOGD("clock re-sync timer expired");
        syncClock();
    };

    // The largest free block matters as much as the free heap
    static auto heapCallback = []() {
        LOGI("heap: free=%u, max block=%u, fragmentation=%u%%",
             ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getHeapFragmentation());
    };
    heapCallback();

    td.startTimer(heapCallback, (15 * app::MINUTES));
    td.startTimer(clockSyncCallback, (60 * app::MINUTES));
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
    // The meter itself is read on the first timer, show the stored total until then
    LOG("Pulsar heat meter (cal): %u", static_cast<unsigned>(config.getOr<PersistCounter>(HEAT_ENERGY, 0)));
    LOG("Pzem power meter (Wh): %u", pzemPowerMeter.getValue());
}

void loop() {

    if (waterColdCounter.process())
    {
        LOGD("process cold water counter");

        auto &config = config::getInstance();
        config.set<PersistCounter>(COLD_WATER, waterColdCounter.getValue());
        config.write(configJournal);
    }

    if (waterHotCounter.process())
    {
        LOGD("process hot water counter");

        auto &config = config::getInstance();
        config.set<PersistCounter>(HOT_WATER, waterHotCounter.getValue());
        config.write(configJournal);
    }

    web.process();
    td.process();

    delay(TICK_MS);
}