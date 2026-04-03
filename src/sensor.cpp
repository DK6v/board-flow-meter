#undef LOG_MODULE
#define LOG_MODULE "DLS"

#include <macros.h>

#include "sensor.h"
#include "WmConfig.h"

using namespace reporter;

namespace app {

// --------------------------------------------------------

DSSensorPin::DSSensorPin(uint8_t pin, Reporter& reporter)
  : PinBase(pin),
    mOneWire(pin),
    mReporter(reporter),
    mSensors()
{
    pinMode(pin, INPUT);
}

std::list<DSSensorPin::Sensor>::const_iterator DSSensorPin::begin() const
{
    return mSensors.begin();
}

std::list<DSSensorPin::Sensor>::const_iterator DSSensorPin::end() const
{
    return mSensors.end();
}

void DSSensorPin::search() {
    mOneWire.reset_search();
    mSensors.clear();

    Sensor sensor;
    while(true == mOneWire.search(sensor)) {
        mSensors.push_back(sensor);

        Serial.print("Sensor search: ");
        Serial.println(static_cast<std::string>(sensor).c_str());
    }
}

void DSSensorPin::read() {

    DallasTemperature bus(&mOneWire);

    bus.begin();
    bus.requestTemperatures();

    for (auto & sensor : mSensors) {

        sensor.tempC = bus.getTempC(sensor);

        Serial.print("Sensor read: ");
        Serial.print(static_cast<std::string>(sensor).c_str());
        Serial.print(", temperature(C): ");
        Serial.println(sensor.tempC);
    }
}

app::Result DSSensorPin::get(std::string addr, float* out) const {
    for (auto & sensor : mSensors) {
        if (addr.compare(addr) == 0) {
            *out = sensor.tempC;
            return app::RESULT_OK;
        }
    }
    return app::RESULT_NOENT;
}

std::string DSSensorPin::Sensor::address() const
{
    std::stringstream ss;
    for (auto it = mAddress.rbegin(); it != mAddress.rend(); ++it)
    {
        ss << std::hex << std::uppercase << (int)*it;
    }
    return ss.str();
}

void DSSensorPin::addParameters(WiFiManager& wm) {
    for (auto & sensor : mSensors) {
        sensor.addParameters(wm);
    }
}

void DSSensorPin::Sensor::addParameters(WiFiManager& wm) {

    mParamHeader = std::string("Sensor ") +
                   static_cast<std::string>(*this) +
                   std::string("<hr><br/>");

    CustomText paramHeader(mParamHeader.c_str());
    wm.addParameter( &paramHeader );
}

void DSSensorPin::send(Reporter& reporter) {

    for (auto & sensor: mSensors)
    {
        LOGI("temperature: address=%s, value=%g", sensor.address().c_str(), ROUNDF(sensor.tempC, 2));

        mReporter.clear();
        mReporter.addTag("device_type", "dallas");
        mReporter.addTag("device_id", sensor.address().c_str());
        mReporter.addTag("entity_id", "temperature");
        mReporter.addTag("entity_type", "gauge");
        mReporter.addTag("entity_unit", "C");
        mReporter.addField("value", ROUNDF(sensor.tempC, 2));
        mReporter.send();
    }
}

void DSSensorPin::onTimer() {
    read();
    send(mReporter);
}

} // namespace fm