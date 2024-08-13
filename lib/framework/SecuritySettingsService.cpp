#include <SecuritySettingsService.h>

#if FT_ENABLED(FT_SECURITY)

SecuritySettingsService::SecuritySettingsService(AsyncWebServer* server, FS* fs) 
  :
    _httpEndpoint
    {
      SecuritySettings::read, 
      SecuritySettings::update, 
      this, 
      server, 
      SECURITY_SETTINGS_PATH, 
      this
    },
    _fsPersistence
    {
      SecuritySettings::read, 
      SecuritySettings::update, 
      this, 
      fs, 
      SECURITY_SETTINGS_FILE
    },
    _jwtHandler{FACTORY_JWT_SECRET} 
{
  addUpdateHandler([&](const char* originId) { configureJWTHandler(); }, false);
}

void SecuritySettingsService::begin() 
{
  _fsPersistence.readFromFS();
  configureJWTHandler();
}

Authentication SecuritySettingsService::authenticateRequest(AsyncWebServerRequest* request) 
{
  const AsyncWebHeader* authorizationHeader = request->getHeader(AUTHORIZATION_HEADER);
  
  if (authorizationHeader) 
  {
    const char* value = authorizationHeader->value().c_str();
    Serial.println(value);
    if (strncmp(value, AUTHORIZATION_HEADER_PREFIX, strlen(AUTHORIZATION_HEADER_PREFIX)) == 0) {
      return authenticateJWT(value + strlen(AUTHORIZATION_HEADER_PREFIX));
    }
  } 
  else if (request->hasParam(ACCESS_TOKEN_PARAMATER)) 
  {
    const AsyncWebParameter* tokenParameter = request->getParam(ACCESS_TOKEN_PARAMATER);
    const char* value = tokenParameter->value().c_str();
    return authenticateJWT(value);
  }

  return Authentication();
}

void SecuritySettingsService::configureJWTHandler() 
{
  _jwtHandler.setSecret(_state.jwtSecret);
}

Authentication SecuritySettingsService::authenticateJWT(const char* jwt) 
{
  JsonDocument payloadDocument;
  _jwtHandler.parseJWT(jwt, strlen(jwt), payloadDocument);

  serializeJson(payloadDocument, Serial);

  if (payloadDocument.is<JsonObject>()) 
  {
    JsonObject parsedPayload = payloadDocument.as<JsonObject>();
    const char* username = parsedPayload["username"];
    for (User& _user : _state.users) 
    {
      if (strcmp(_user.username, username) == 0 && validatePayload(parsedPayload, &_user)) {
        return Authentication(_user);
      }
    }
  }

  return Authentication();
}

Authentication SecuritySettingsService::authenticate(const char* username, const char* password) 
{
  for (User& _user : _state.users) 
  {
    if (strcmp(_user.username, username) == 0 && strcmp(_user.password, password) == 0) 
    {
      return Authentication(_user);
    }
  }

  return Authentication();
}

inline void populateJWTPayload(JsonObject& payload, User* user) 
{
  payload["username"] = user->username;
  payload["admin"] = user->admin;
}

boolean SecuritySettingsService::validatePayload(JsonObject& parsedPayload, User* user) 
{
  JsonDocument json;
  JsonObject payload = json.to<JsonObject>();
  populateJWTPayload(payload, user);
  return payload == parsedPayload;
}

void SecuritySettingsService::generateJWT(User* user, char* jwt, size_t len) 
{
  JsonDocument json;
  JsonObject payload = json.to<JsonObject>();
  populateJWTPayload(payload, user);
  _jwtHandler.buildJWT(payload, jwt, len);
}

ArRequestFilterFunction SecuritySettingsService::filterRequest(AuthenticationPredicate predicate) 
{
  return [this, predicate](AsyncWebServerRequest* request) {
    Authentication authentication = authenticateRequest(request);
    return predicate(authentication);
  };
}

ArRequestHandlerFunction SecuritySettingsService::wrapRequest(ArRequestHandlerFunction onRequest,
                                                              AuthenticationPredicate predicate) 
{
  return [this, onRequest, predicate](AsyncWebServerRequest* request) {
    Authentication authentication = authenticateRequest(request);
    if (!predicate(authentication)) {
      request->send(401);
      return;
    }
    onRequest(request);
  };
}

ArJsonRequestHandlerFunction SecuritySettingsService::wrapCallback(ArJsonRequestHandlerFunction onRequest,
                                                                   AuthenticationPredicate predicate) 
{
  return [this, onRequest, predicate](AsyncWebServerRequest* request, JsonVariant& json) {
    Authentication authentication = authenticateRequest(request);
    if (!predicate(authentication)) {
      request->send(401);
      return;
    }
    onRequest(request, json);
  };
}

#else

User ADMIN_USER = User(FACTORY_ADMIN_USERNAME, FACTORY_ADMIN_PASSWORD, true);

SecuritySettingsService::SecuritySettingsService(AsyncWebServer* server, FS* fs) : SecurityManager() {
}
SecuritySettingsService::~SecuritySettingsService() {
}

ArRequestFilterFunction SecuritySettingsService::filterRequest(AuthenticationPredicate predicate) {
  return [this, predicate](AsyncWebServerRequest* request) { return true; };
}

// Return the admin user on all request - disabling security features
Authentication SecuritySettingsService::authenticateRequest(AsyncWebServerRequest* request) {
  return Authentication(ADMIN_USER);
}

// Return the function unwrapped
ArRequestHandlerFunction SecuritySettingsService::wrapRequest(ArRequestHandlerFunction onRequest,
                                                              AuthenticationPredicate predicate) {
  return onRequest;
}

ArJsonRequestHandlerFunction SecuritySettingsService::wrapCallback(ArJsonRequestHandlerFunction onRequest,
                                                                   AuthenticationPredicate predicate) {
  return onRequest;
}

#endif
