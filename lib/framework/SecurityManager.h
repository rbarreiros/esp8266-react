#ifndef SecurityManager_h
#define SecurityManager_h

#include <Features.h>
#include <ArduinoJsonJWT.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <list>

#define ACCESS_TOKEN_PARAMATER "access_token"

#define AUTHORIZATION_HEADER "Authorization"
#define AUTHORIZATION_HEADER_PREFIX "Bearer "
#define AUTHORIZATION_HEADER_PREFIX_LEN 7

#define SECURITY_USERNAME_SIZE 32
#define SECURITY_PASSWORD_SIZE 64

class User 
{
public:
  char username[SECURITY_USERNAME_SIZE];
  char password[SECURITY_PASSWORD_SIZE];
  bool admin;

 public:
  User(const char* username, const char* password, bool admin) : admin(admin) 
  {
    strncpy(this->username, username, SECURITY_USERNAME_SIZE);
    strncpy(this->password, password, SECURITY_PASSWORD_SIZE);
  }
};

class Authentication 
{
public:
  User* user;
  boolean authenticated;

 public:
  Authentication(User& user) : user(new User(user)), authenticated(true) 
  {}
  
  Authentication() : user(nullptr), authenticated(false)
  {}
  
  ~Authentication() 
  {
    delete (user);
  }
};

typedef std::function<boolean(Authentication& authentication)> AuthenticationPredicate;

class AuthenticationPredicates 
{
public:
  static bool NONE_REQUIRED(Authentication& authentication) 
  {
    return true;
  };

  static bool IS_AUTHENTICATED(Authentication& authentication) 
  {
    return authentication.authenticated;
  };

  static bool IS_ADMIN(Authentication& authentication) 
  {
    return authentication.authenticated && authentication.user->admin;
  };
};

class SecurityManager 
{
public:
#if FT_ENABLED(FT_SECURITY)
  /*
   * Authenticate, returning the user if found
   */
  virtual Authentication authenticate(const char* username, const char* password) = 0;

  /*
   * Generate a JWT for the user provided
   */
  virtual void generateJWT(User* user, char* jwt, size_t len) = 0;
#endif

  /*
   * Check the request header for the Authorization token
   */
  virtual Authentication authenticateRequest(AsyncWebServerRequest* request) = 0;

  /**
   * Filter a request with the provided predicate, only returning true if the predicate matches.
   */
  virtual ArRequestFilterFunction filterRequest(AuthenticationPredicate predicate) = 0;

  /**
   * Wrap the provided request to provide validation against an AuthenticationPredicate.
   */
  virtual ArRequestHandlerFunction wrapRequest(ArRequestHandlerFunction onRequest,
                                               AuthenticationPredicate predicate) = 0;

  /**
   * Wrap the provided json request callback to provide validation against an AuthenticationPredicate.
   */
  virtual ArJsonRequestHandlerFunction wrapCallback(ArJsonRequestHandlerFunction onRequest,
                                                    AuthenticationPredicate predicate) = 0;
};

#endif  // end SecurityManager_h
