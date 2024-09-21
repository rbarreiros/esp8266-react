#include "SystemSettingsService.h"


SystemSettingsService::SystemSettingsService(
    AsyncWebServer* server,
    APSettingsService *ap,
    FactoryResetService *fr,
    FS *fs,
    SecurityManager *security
)
    :
    m_httpEndpoint
    {
        SystemSettings::read,
        SystemSettings::update,
        this,
        server,
        SYSTEM_SETTINGS_SERVICE_PATH,
        security
    },
    m_ap{ap},
    m_fr{fr},
    m_fsPersistence
    {
        SystemSettings::read,
        SystemSettings::update,
        this,
        fs,
        SYSTEM_SETTINGS_FILE
    },
    m_lastResetButtonState{RESET_BUTTON_OFF},
    m_lastResetButtonPushed{0},
    m_toggleLed{false},
    m_toggleTime{WIFI_LED_LONG_TOGGLE},
    m_lastWifiLedTick{0}
{
}

void SystemSettingsService::begin()
{
    m_fsPersistence.readFromFS();
    configureSystemSettings();
}

void SystemSettingsService::loop()
{
    if(_state.resetEnabled)
        processResetButton();

    if(_state.wifiLedEnabled)
        processWiFiLed();

    if(m_toggleLed)
        toggleLed();
}

void SystemSettingsService::configureSystemSettings()
{
    // configure pins
    if(_state.resetEnabled)
    {
        pinMode(_state.resetPin, FACTORY_SYSTEM_RESET_BUTTON_PULLUP ? INPUT_PULLUP : INPUT);
    }

    if(_state.wifiLedEnabled)
    {
        pinMode(_state.wifiLedPin, OUTPUT);
        digitalWrite(_state.wifiLedPin, WIFI_LED_OFF);
    }
}

void SystemSettingsService::processResetButton()
{

    if((m_lastResetButtonState == RESET_BUTTON_OFF) 
        && (digitalRead(_state.resetPin) == RESET_BUTTON_ON))
    { // First press
        m_lastResetButtonState = RESET_BUTTON_ON;
        m_lastResetButtonPushed = millis();
    }
    else if((m_lastResetButtonState == RESET_BUTTON_ON)
        && (digitalRead(_state.resetPin) == RESET_BUTTON_ON))
    { // Continuing to push
        if((millis() - m_lastResetButtonPushed) > _state.resetTime)
        {
            m_fr->factoryReset();
        }
    }
    else if((m_lastResetButtonState == RESET_BUTTON_ON)
        && (digitalRead(_state.resetPin) == RESET_BUTTON_OFF))
    {  // stopped pressing before required time
        m_lastResetButtonState = RESET_BUTTON_OFF;
    }
}

void SystemSettingsService::processWiFiLed()
{
    APSettings ap;

    m_ap->read([&](APSettings& state) { ap = state; });

    // Wifi connected, led on
    if(WiFi.status() == WL_CONNECTED)
    {
        m_toggleLed = false;
        digitalWrite(_state.wifiLedPin, WIFI_LED_ON);
    }
    // AP mode is always on, 
    // if wifi is disconnected, long blink
    else if(ap.provisionMode == AP_MODE_ALWAYS)
    {
        m_toggleLed = true;
        m_toggleTime = WIFI_LED_LONG_TOGGLE;
    }
    // AP Mode is on, will disconnect after
    // wifi connection
    else if(ap.provisionMode == AP_MODE_DISCONNECTED)
    {
        m_toggleLed = true;
        m_toggleTime = WIFI_LED_SHORT_TOGGLE;
    }
    // AP mode off, and wifi is disconnected
    else if(ap.provisionMode == AP_MODE_NEVER)
    {
        m_toggleLed = true;
        m_toggleTime = WIFI_LED_VERY_SHORT_TOGGLE;
    }
}

void SystemSettingsService::toggleLed()
{
    if((millis() - m_lastWifiLedTick) > m_toggleTime)
    {
        m_lastWifiLedTick = millis();
        digitalWrite(_state.wifiLedPin,
                     !digitalRead(_state.wifiLedPin));
    }
}