#include "RemoteSettingsService.h"

void debugRemote(Remote rem)
{
    Serial.printf("Remote: \r\nID: %s\tButton: %d\tDescription: %s\tSerial: %s\r\n",
        rem.id(), rem.button, rem.description, rem.getSerial());
}

RemoteSettingsService::RemoteSettingsService(AsyncWebServer* server,
                                             SecurityManager* security,
                                             FS* fs,
                                             GarageStateService* garage) 
    :
    m_httpEndpoint{RemoteSettings::read,
                   RemoteSettings::update,
                   this,
                   server,
                   REMOTE_SETTINGS_ENDPOINT_PATH,
                   security,
                   AuthenticationPredicates::IS_AUTHENTICATED},
    m_fs{RemoteSettings::readFs,
         RemoteSettings::updateFs,  // should only be used for batch, but, most likely not used
         this,
         fs,
         REMOTE_SETTINGS_FILE},
    m_security{security},
    m_garageService{garage},
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
            AuthenticationPredicates::IS_ADMIN)} 
{
    m_fs.disableUpdateHandler();

    // Register CRUD
    server->on(REMOTE_CRUD_READ_ENDPOINT_PATH,
             HTTP_GET,
             security->wrapRequest(std::bind(&RemoteSettingsService::readRemote, this, std::placeholders::_1),
                                   AuthenticationPredicates::IS_ADMIN));

    server->addHandler(&m_createHandler);
    server->addHandler(&m_updateHandler);
    server->addHandler(&m_deleteHandler);
}

void RemoteSettingsService::begin() 
{
    m_fs.readFromFS();
    //_state.pairingTimeout = 60000;
}

const char* RemoteSettingsService::getDescription(RemotePacket packet, RemoteSerial serial)
{
    for (auto& remote : _state.remotes)
    {
        if (remote.button == packet.button && remote.serial == serial)
        {
            return remote.description;
        }
    }
    return nullptr;
}

bool RemoteSettingsService::isValid(RemotePacket packet, RemoteSerial serial)
{
   for (auto& remote : _state.remotes)
    {
        if (remote.button == packet.button && remote.serial == serial)
        {
            return true;
        }
    }
    return false;
}

bool RemoteSettingsService::addRemote(RemotePacket packet, RemoteSerial serial)
{
    char description[REMOTE_MAX_DESCRIPTION_SIZE];
    snprintf(description, sizeof(description), "Remote %02X%02X%02X%02X%02X",
             packet.button, serial.ser[0], serial.ser[1], serial.ser[2], serial.ser[3]);
    return addRemote(packet, serial, description);
}

bool RemoteSettingsService::addRemote(RemotePacket packet, RemoteSerial serial, const char* description)
{
    Remote rem = getRemote(packet, serial);
    if(rem.button != 0)
        return false;

    Remote remote(packet.button, description, serial);
    
    debugRemote(remote);

    update([&](RemoteSettings& settings) {
        settings.remotes.push_back(remote);
        return StateUpdateResult::CHANGED;
    }, "addremote");

    m_fs.writeToFS();
    return true;
}

bool RemoteSettingsService::editRemote(const char* id, const char* description)
{
    for(auto& remote : _state.remotes)
    {
        if(strcmp(remote.id(), id) == 0)
        {
            strncpy(remote.description, description, REMOTE_MAX_DESCRIPTION_SIZE - 1);
            remote.description[REMOTE_MAX_DESCRIPTION_SIZE - 1] = '\0';

            update([&](RemoteSettings& settings) {
                return StateUpdateResult::CHANGED;
            }, "editremote");
            
            m_fs.writeToFS();
            return true;
        }
    }

    return false;
}

Remote RemoteSettingsService::getRemote(const char* id)
{
    for (auto& remote : _state.remotes)
    {
        if (strcmp(remote.id(), id) == 0)
        {
            return remote;
        }
    }
    return Remote();
}

Remote RemoteSettingsService::getRemote(RemotePacket packet, RemoteSerial serial)
{
    for(auto& remote : _state.remotes)
    {
        if((remote.isEqual(serial)) && (packet.button == remote.button))
            return remote;
    }

    return Remote{};
}

bool RemoteSettingsService::delRemote(const char* id)
{
    int i = 0;
    for (auto it = _state.remotes.begin(); it != _state.remotes.end(); ++it)
    {
        if (strcmp(it->id(), id) == 0)
        {
            _state.remotes.erase(it);
            update([&](RemoteSettings& settings) {
                return StateUpdateResult::CHANGED;
            }, "deleteremote");
            
            m_fs.writeToFS();
            return true;
        }
        i++;
    }

    return false;
}

void RemoteSettingsService::createRemote(AsyncWebServerRequest *request, JsonVariant &json)
{
    if (json.is<JsonObject>())
    {
        JsonObject jsonObj = json.as<JsonObject>();
        const char* description = jsonObj["description"] | "";
        const char* serial = jsonObj["serial"] | "";
        uint8_t button = jsonObj["button"] | 0;

        RemotePacket remotePacket;
        remotePacket.button = button;

        Remote remote;
        remote.setSerial(serial);
        if (addRemote(remotePacket, remote.serial, description))
        {
            request->send(200, "application/json", "{\"success\":true}");
        }
        else
        {
            request->send(500, "application/json", "{\"success\":false, \"message\":\"Failed to add remote\"}");
        }
    }
    else
    {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"Invalid JSON\"}");
    }
}

void RemoteSettingsService::readRemote(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (auto& remote : _state.remotes)
    {
        JsonObject obj = array.add<JsonObject>();
        obj["id"] = remote.id();
        obj["button"] = remote.button;
        obj["description"] = remote.description;
        obj["serial"] = remote.getSerial();
    }

    serializeJson(doc, *response);
    request->send(response);
}

void RemoteSettingsService::updateRemote(AsyncWebServerRequest *request, JsonVariant &json)
{
    if (json.is<JsonObject>())
    {
        JsonObject jsonObj = json.as<JsonObject>();
        const char* id = jsonObj["id"] | "";
        const char* description = jsonObj["description"] | "";

        if (editRemote(id, description))
        {
            request->send(200, "application/json", "{\"success\":true}");
        }
        else
        {
            request->send(404, "application/json", "{\"success\":false, \"message\":\"Remote not found\"}");
        }
    }
    else
    {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"Invalid JSON\"}");
    }
}

void RemoteSettingsService::deleteRemote(AsyncWebServerRequest *request, JsonVariant &json)
{
    if (json.is<JsonObject>())
    {
        JsonObject jsonObj = json.as<JsonObject>();
        const char* id = jsonObj["id"] | "";

        if (delRemote(id))
        {
            request->send(200, "application/json", "{\"success\":true}");
        }
        else
        {
            request->send(404, "application/json", "{\"success\":false, \"message\":\"Remote not found\"}");
        }
    }
    else
    {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"Invalid JSON\"}");
    }
}
