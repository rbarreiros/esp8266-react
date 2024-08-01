#ifndef _SYSTEMSETTINGSSERVICE_H_
#define _SYSTEMSETTINGSSERVICE_H_

#include <StatefulService.h>
#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <APSettingsService.h>
#include <FactoryResetService.h>

#ifndef FACTORY_SYSTEM_RESET_BUTTON
#define FACTORY_SYSTEM_RESET_BUTTON false
#endif

#ifndef FACTORY_SYSTEM_RESET_BUTTON_PIN
#define FACTORY_SYSTEM_RESET_BUTTON_PIN 0
#endif

#ifndef FACTORY_SYSTEM_RESET_BUTTON_PULLUP
#define FACTORY_SYSTEM_RESET_BUTTON_PULLUP true
#endif

#ifndef FACTORY_SYSTEM_RESET_TIME
#define FACTORY_SYSTEM_RESET_TIME 5000
#endif

#ifndef FACTORY_SYSTEM_WIFI_LED
#define FACTORY_SYSTEM_WIFI_LED true
#endif

#ifndef FACTORY_SYSTEM_WIFI_LED_PIN
#define FACTORY_SYSTEM_WIFI_LED_PIN 0
#endif

#ifndef FACTORY_SYSTEM_WIFI_LED_SINK
#define FACTORY_SYSTEM_WIFI_LED_SINK true
#endif

#define SYSTEM_SETTINGS_FILE "/config/systemSettings.json"
#define SYSTEM_SETTINGS_SERVICE_PATH "/rest/systemSettings"

#define RESET_BUTTON_ON  FACTORY_SYSTEM_RESET_BUTTON_PULLUP ? LOW : HIGH
#define RESET_BUTTON_OFF FACTORY_SYSTEM_RESET_BUTTON_PULLUP ? HIGH : LOW

#define WIFI_LED_ON  FACTORY_SYSTEM_WIFI_LED_SINK ? LOW : HIGH
#define WIFI_LED_OFF FACTORY_SYSTEM_WIFI_LED_SINK ? LOW : HIGH

#define WIFI_LED_LONG_TOGGLE 1000
#define WIFI_LED_SHORT_TOGGLE 500
#define WIFI_LED_VERY_SHORT_TOGGLE 250

class SystemSettings
{
public:
    bool resetEnabled;
    uint8_t resetPin;
    bool resetPullUp;
    unsigned long resetTime;

    bool wifiLedEnabled;
    uint8_t wifiLedPin;
    bool wifiLedSink;

    static void read(SystemSettings& settings, JsonObject& root)
    {
        root["reset_enabled"] = settings.resetEnabled;
        root["reset_pin"] = settings.resetPin;
        root["reset_pullup"] = settings.resetPullUp;
        root["reset_time"] = settings.resetTime;

        root["wifi_led_enabled"] = settings.wifiLedEnabled;
        root["wifi_led_pin"] = settings.wifiLedPin;
        root["wifi_led_sink"] = settings.wifiLedSink;
    }

    static StateUpdateResult update(JsonObject& root, SystemSettings& settings)
    {
        settings.resetEnabled = root["reset_enabled"] | FACTORY_SYSTEM_RESET_BUTTON;
        settings.resetPin = root["reset_pin"] | FACTORY_SYSTEM_RESET_BUTTON_PIN;
        settings.resetPullUp = root["reset_pullup"] | FACTORY_SYSTEM_RESET_BUTTON_PULLUP;
        settings.resetTime = root["reset_time"] | FACTORY_SYSTEM_RESET_TIME;

        settings.wifiLedEnabled = root["wifi_led_enabled"] | FACTORY_SYSTEM_WIFI_LED;
        settings.wifiLedPin = root["wifi_led_pin"] | FACTORY_SYSTEM_WIFI_LED_PIN;
        settings.wifiLedSink = root["wifi_led_sing"] | FACTORY_SYSTEM_WIFI_LED_SINK;

        return StateUpdateResult::CHANGED;
    }
};

class SystemSettingsService : public StatefulService<SystemSettings>
{
public:
    SystemSettingsService(AsyncWebServer* server, 
                          APSettingsService *ap,
                          FactoryResetService *fr,
                          FS *fs, 
                          SecurityManager *security);

    void begin();
    void loop();

private:
    HttpEndpoint<SystemSettings>    m_httpEndpoint;
    APSettingsService*              m_ap;
    FactoryResetService*            m_fr;
    FSPersistence<SystemSettings>   m_fsPersistence;

    uint8_t m_lastResetButtonState;
    unsigned long m_lastResetButtonPushed;
    bool m_toggleLed;
    unsigned long m_toggleTime;
    unsigned long m_lastWifiLedTick;
    

    void configureSystemSettings();
    void processResetButton();
    void processWiFiLed();
    void toggleLed();
};

#endif
