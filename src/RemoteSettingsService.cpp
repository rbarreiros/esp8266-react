#include "RemoteSettingsService.h"

RemoteSettingsService::RemoteSettingsService(AsyncWebServer* server,
                                             SecurityManager* security,
                                             FS* fs,
                                             GarageStateService* garage,
                                             RfRemoteController* remoteCtrl) :
    m_httpEndpoint{RemoteSettings::read,
                   RemoteSettings::update,
                   this,
                   server,
                   REMOTE_SETTINGS_ENDPOINT_PATH,
                   security,
                   AuthenticationPredicates::IS_AUTHENTICATED},
    m_webSocket{RemoteSettings::read,
                RemoteSettings::update,
                this,
                server,
                REMOTE_SETTINGS_SOCKET_PATH,
                security,
                AuthenticationPredicates::IS_ADMIN},
    m_fs{RemoteSettings::read,
         RemoteSettings::update,  // should only be used for batch, but, most likely not used
         this,
         fs,
         REMOTE_SETTINGS_FILE},
    m_security{security},
    m_garageService{garage},
    m_remoteCtrl{remoteCtrl},
    m_createHandler{
        REMOTE_CRUD_CREATE_ENDPOINT_PATH,
        security->wrapCallback(
            std::bind(&RemoteSettingsService::createRemote, this, std::placeholders::_1, std::placeholders::_2),
            AuthenticationPredicates::IS_ADMIN)},
    m_updateHandler{
        REMOTE_CRUD_UPDATE_ENDPOINT_PATH,
        security->wrapCallback(
            std::bind(&RemoteSettingsService::updateRemote, this, std::placeholders::_1, std::placeholders::_2),
            AuthenticationPredicates::IS_ADMIN)},
    m_deleteHandler{
        REMOTE_CRUD_DELETE_ENDPOINT_PATH,
        security->wrapCallback(
            std::bind(&RemoteSettingsService::deleteRemote, this, std::placeholders::_1, std::placeholders::_2),
            AuthenticationPredicates::IS_ADMIN)} {
  // Register CRUD
  server->on(REMOTE_CRUD_READ_ENDPOINT_PATH,
             HTTP_GET,
             security->wrapRequest(std::bind(&RemoteSettingsService::readRemote, this, std::placeholders::_1),
                                   AuthenticationPredicates::IS_ADMIN));

  server->addHandler(&m_createHandler);
  server->addHandler(&m_updateHandler);
  server->addHandler(&m_deleteHandler);

  // When a remote code is received, it calls this method
  //m_remoteCtrl->addCallback(
    //std::bind(&RemoteSettingsService::onRemoteReceived, this, std::placeholders::_1, std::placeholders::_2));
}

void RemoteSettingsService::begin() {
}

String RemoteSettingsService::getDescription(RemotePacket packet, RemoteSerial serial)
{
    return "Test description";
}

bool RemoteSettingsService::isValid(RemotePacket packet, RemoteSerial serial)
{
    return true;
}


void RemoteSettingsService::createRemote(AsyncWebServerRequest* request, JsonVariant& json) {
}

void RemoteSettingsService::readRemote(AsyncWebServerRequest* request) {
}

void RemoteSettingsService::updateRemote(AsyncWebServerRequest* request, JsonVariant& json) {
}

void RemoteSettingsService::deleteRemote(AsyncWebServerRequest* request, JsonVariant& json) {
}

/*
void RemoteSettingsService::onRemoteReceived(RemotePacket packet, RemoteSerial serial) {
  // Check if serial is in our database, if it is, return its reccord
    Serial.printf("Received button press %d from serial %02x %02x %02x %02x\r\n",
        packet.button, serial.ser[0], serial.ser[1], serial.ser[2], serial.ser[3]);
  // Is it the correct button ?

  // great then, trigger relay
  
    m_garageService->update(
        [](GarageState& state) {
            state.relayOn = true;
            return StateUpdateResult::CHANGED;
        },
    "remotes");
}
*/