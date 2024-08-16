#include "RemoteSettingsService.h"

void debugRemote(Remote rem)
{
    Serial.printf("Remote: \r\nID: %s\tButton: %d\tDescription: %s\tSerial: %s\r\n",
        rem.id().c_str(), rem.button, rem.description.c_str(), rem.getSerial().c_str());
}

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
    m_fs{RemoteSettings::readFs,
         RemoteSettings::updateFs,  // should only be used for batch, but, most likely not used
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
}

void RemoteSettingsService::begin() {
    m_fs.readFromFS();
}

String RemoteSettingsService::getDescription(RemotePacket packet, RemoteSerial serial)
{
    for (auto& remote : _state.remotes)
    {
        if (remote.button == packet.button && remote.serial == serial)
        {
            return remote.description;
        }
    }
    return "";
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
    return addRemote(packet, serial, "Remote");
}

bool RemoteSettingsService::addRemote(RemotePacket packet, RemoteSerial serial, String description)
{
    Remote rem = getRemote(packet, serial);
    if(rem.button != 0)
        return false;

    //Remote remote(packet.button, description, serial);
    rem.button = packet.button;
    rem.description = description;
    rem.serial = serial;
    
    debugRemote(rem);
    _state.remotes.push_back(rem);

    update([&](RemoteSettings& settings) {
        return StateUpdateResult::CHANGED;
    }, "addremote");

    m_fs.writeToFS();

    for(auto& remote : _state.remotes)
        debugRemote(remote);

    return true;
}

bool RemoteSettingsService::editRemote(String id, String description)
{
    for(auto& remote : _state.remotes)
    {
        if(id == remote.id())
        {
            remote.description = description;

            update([&](RemoteSettings& settings) {
                return StateUpdateResult::CHANGED;
            }, "editremote");
            
            m_fs.writeToFS();
            return true;
        }
    }

    return false;
}

Remote RemoteSettingsService::getRemote(String id)
{
    for (auto& remote : _state.remotes)
    {
        if (id = remote.id())
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

bool RemoteSettingsService::delRemote(String id)
{
    int i = 0;
    for (auto it = _state.remotes.begin(); it != _state.remotes.end(); ++it)
    {
        if(it->id() == id)
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

void RemoteSettingsService::createRemote(AsyncWebServerRequest* request, JsonVariant& json) 
{
    if (json.is<JsonObject>())
    {
        JsonObject jsonObj = json.as<JsonObject>();

        String description = jsonObj["description"] | "";
        String serial = jsonObj["serial"] | "";

        RemotePacket remotePacket;
        remotePacket.button = jsonObj["button"] | 0;

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

void RemoteSettingsService::readRemote(AsyncWebServerRequest* request) {
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

void RemoteSettingsService::updateRemote(AsyncWebServerRequest* request, JsonVariant& json) {
    if (json.is<JsonObject>())
    {
        JsonObject jsonObj = json.as<JsonObject>();
        String id = jsonObj["id"] | "";
        String description = jsonObj["description"] | "";

        Serial.printf("Updating %s - %s\r\n", id.c_str(), description.c_str());

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

void RemoteSettingsService::deleteRemote(AsyncWebServerRequest* request, JsonVariant& json) {
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
