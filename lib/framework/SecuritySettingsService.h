#ifndef SecuritySettingsService_h
#define SecuritySettingsService_h

#include <SettingValue.h>
#include <Features.h>
#include <SecurityManager.h>
#include <HttpEndpoint.h>
#include <FSPersistence.h>

#ifndef FACTORY_JWT_SECRET
#define FACTORY_JWT_SECRET "#{random}-#{random}"
#endif

#ifndef FACTORY_ADMIN_USERNAME
#define FACTORY_ADMIN_USERNAME "admin"
#endif

#ifndef FACTORY_ADMIN_PASSWORD
#define FACTORY_ADMIN_PASSWORD "admin"
#endif

#ifndef FACTORY_GUEST_USERNAME
#define FACTORY_GUEST_USERNAME "guest"
#endif

#ifndef FACTORY_GUEST_PASSWORD
#define FACTORY_GUEST_PASSWORD "guest"
#endif

#define SECURITY_SETTINGS_FILE "/config/securitySettings.json"
#define SECURITY_SETTINGS_PATH "/rest/securitySettings"

#if FT_ENABLED(FT_SECURITY)

#define JWT_SECRET_LENGTH 128
#define USERNAME_LENGTH 32
#define PASSWORD_LENGTH 64

class SecuritySettings 
{
public:
  char jwtSecret[JWT_SECRET_LENGTH];
  std::list<User> users;

  static void read(SecuritySettings& settings, JsonObject& root) 
  {
    // secret
    root["jwt_secret"] = settings.jwtSecret;

    // users
    //JsonArray users = root.createNestedArray("users");
    JsonArray users = root["users"].to<JsonArray>();
    for (User user : settings.users) 
    {
      //JsonObject userRoot = users.createNestedObject();
      JsonObject userRoot = users.add<JsonObject>();
      userRoot["username"] = user.username;
      userRoot["password"] = user.password;
      userRoot["admin"] = user.admin;
    }
  }

  static StateUpdateResult update(JsonObject& root, SecuritySettings& settings) 
  {
    // secret
    const char* jwtSecret = root["jwt_secret"].as<const char*>();
    if(!jwtSecret || strlen(jwtSecret) == 0)
      jwtSecret = const_cast<const char*>(SettingValue::format(FACTORY_JWT_SECRET));

    strncpy(settings.jwtSecret, jwtSecret, JWT_SECRET_LENGTH - 1);
    settings.jwtSecret[JWT_SECRET_LENGTH - 1] = '\0';

    // users
    settings.users.clear();
    if (root["users"].is<JsonArray>()) {
      for (JsonVariant user : root["users"].as<JsonArray>()) {
        settings.users.push_back(User(user["username"].as<const char*>(), user["password"].as<const char*>(), user["admin"]));
      }
    } else {
      settings.users.push_back(User(FACTORY_ADMIN_USERNAME, FACTORY_ADMIN_PASSWORD, true));
      settings.users.push_back(User(FACTORY_GUEST_USERNAME, FACTORY_GUEST_PASSWORD, false));
    }
    return StateUpdateResult::CHANGED;
  }
};

class SecuritySettingsService : public StatefulService<SecuritySettings>, public SecurityManager 
{
public:
  SecuritySettingsService(AsyncWebServer* server, FS* fs);

  void begin();

  // Functions to implement SecurityManager
  Authentication authenticate(const char* username, const char* password);
  Authentication authenticateRequest(AsyncWebServerRequest* request);
  void generateJWT(User* user, char* jwt, size_t len);
  ArRequestFilterFunction filterRequest(AuthenticationPredicate predicate);
  ArRequestHandlerFunction wrapRequest(ArRequestHandlerFunction onRequest, AuthenticationPredicate predicate);
  ArJsonRequestHandlerFunction wrapCallback(ArJsonRequestHandlerFunction callback, AuthenticationPredicate predicate);

 private:
  HttpEndpoint<SecuritySettings> _httpEndpoint;
  FSPersistence<SecuritySettings> _fsPersistence;
  ArduinoJsonJWT _jwtHandler;

  void configureJWTHandler();

  /*
   * Lookup the user by JWT
   */
  Authentication authenticateJWT(const char* jwt);

  /*
   * Verify the payload is correct
   */
  boolean validatePayload(JsonObject& parsedPayload, User* user);
};

#else

class SecuritySettingsService : public SecurityManager {
 public:
  SecuritySettingsService(AsyncWebServer* server, FS* fs);
  ~SecuritySettingsService();

  // minimal set of functions to support framework with security settings disabled
  Authentication authenticateRequest(AsyncWebServerRequest* request);
  ArRequestFilterFunction filterRequest(AuthenticationPredicate predicate);
  ArRequestHandlerFunction wrapRequest(ArRequestHandlerFunction onRequest, AuthenticationPredicate predicate);
  ArJsonRequestHandlerFunction wrapCallback(ArJsonRequestHandlerFunction onRequest, AuthenticationPredicate predicate);
};

#endif  // end FT_ENABLED(FT_SECURITY)
#endif  // end SecuritySettingsService_h
