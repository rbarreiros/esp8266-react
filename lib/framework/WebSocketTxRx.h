#ifndef WebSocketTxRx_h
#define WebSocketTxRx_h

#include <StatefulService.h>
#include <ESPAsyncWebServer.h>
#include <SecurityManager.h>

#define WEB_SOCKET_CLIENT_ID_MSG_SIZE 128

#define WEB_SOCKET_ORIGIN "websocket"
#define WEB_SOCKET_ORIGIN_CLIENT_ID_PREFIX "websocket:"

template <class T>
class WebSocketConnector 
{
protected:
  StatefulService<T>* _statefulService;
  AsyncWebServer* _server;
  AsyncWebSocket _webSocket;

  WebSocketConnector(StatefulService<T>* statefulService,
                     AsyncWebServer* server,
                     const char* webSocketPath,
                     SecurityManager* securityManager,
                     AuthenticationPredicate authenticationPredicate) 
    :
      _statefulService{statefulService}, 
      _server{server}, 
      _webSocket{webSocketPath}
  {
    _webSocket.setFilter(securityManager->filterRequest(authenticationPredicate));
    _webSocket.onEvent(std::bind(&WebSocketConnector::onWSEvent,
                                 this,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3,
                                 std::placeholders::_4,
                                 std::placeholders::_5,
                                 std::placeholders::_6));
    _server->addHandler(&_webSocket);
    _server->on(webSocketPath, HTTP_GET, std::bind(&WebSocketConnector::forbidden, this, std::placeholders::_1));
  }

  WebSocketConnector(StatefulService<T>* statefulService,
                     AsyncWebServer* server,
                     const char* webSocketPath) 
    :
      _statefulService{statefulService}, 
      _server{server}, 
      _webSocket{webSocketPath}
  {
    _webSocket.onEvent(std::bind(&WebSocketConnector::onWSEvent,
                                 this,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3,
                                 std::placeholders::_4,
                                 std::placeholders::_5,
                                 std::placeholders::_6));
    _server->addHandler(&_webSocket);
  }

  virtual void onWSEvent(AsyncWebSocket* server,
                         AsyncWebSocketClient* client,
                         AwsEventType type,
                         void* arg,
                         uint8_t* data,
                         size_t len) = 0;

  char* clientId(AsyncWebSocketClient* client)
  {
    static char buff[64];
    size_t len = snprintf(buff, 64, "%s%d", WEB_SOCKET_ORIGIN_CLIENT_ID_PREFIX, client->id());
    buff[len == 64 ? len - 1 : len] = '\0';

    return buff;
  }

 private:
  void forbidden(AsyncWebServerRequest* request) 
  {
    request->send(403);
  }
};

template <class T>
class WebSocketTx : virtual public WebSocketConnector<T> 
{
public:
  WebSocketTx(JsonStateReader<T> stateReader,
              StatefulService<T>* statefulService,
              AsyncWebServer* server,
              const char* webSocketPath,
              SecurityManager* securityManager,
              AuthenticationPredicate authenticationPredicate = AuthenticationPredicates::IS_ADMIN) 
    :
      WebSocketConnector<T>
      {
        statefulService,
        server,
        webSocketPath,
        securityManager,
        authenticationPredicate
      },
      _stateReader{stateReader}
  {
    WebSocketConnector<T>::_statefulService->addUpdateHandler(
        [&](const char* originId) { transmitData(nullptr, originId); }, false);
  }

  WebSocketTx(JsonStateReader<T> stateReader,
              StatefulService<T>* statefulService,
              AsyncWebServer* server,
              const char* webSocketPath) 
    :
      WebSocketConnector<T>{statefulService, server, webSocketPath}, 
      _stateReader{stateReader} 
  {
    WebSocketConnector<T>::_statefulService->addUpdateHandler(
      [&](const char* originId) { transmitData(nullptr, originId); }, false);
  }

 protected:
  virtual void onWSEvent(AsyncWebSocket* server,
                         AsyncWebSocketClient* client,
                         AwsEventType type,
                         void* arg,
                         uint8_t* data,
                         size_t len) 
  {
    if (type == WS_EVT_CONNECT) {
      // when a client connects, we transmit it's id and the current payload
      transmitId(client);
      transmitData(client, WEB_SOCKET_ORIGIN);
    }
  }

 private:
  JsonStateReader<T> _stateReader;

  void transmitId(AsyncWebSocketClient* client) 
  {
    JsonDocument json;
    JsonObject root = json.to<JsonObject>();
    root["type"] = "id";
    root["id"] = WebSocketConnector<T>::clientId(client);
    size_t len = measureJson(json);
    AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket.makeBuffer(len);
    if (buffer) {
      serializeJson(json, (char*)buffer->get(), len + 1);
      client->text(buffer);
    }
  }

  /**
   * Broadcasts the payload to the destination, if provided. Otherwise broadcasts to all clients except the origin, if
   * specified.
   *
   * Original implementation sent clients their own IDs so they could ignore updates they initiated. This approach
   * simplifies the client and the server implementation but may not be sufficent for all use-cases.
   */
  void transmitData(AsyncWebSocketClient* client, const char* originId) 
  {
    JsonDocument json;
    JsonObject root = json.to<JsonObject>();
    root["type"] = "payload";
    root["origin_id"] = const_cast<char*>(originId);
    //JsonObject payload = root.createNestedObject("payload");
    JsonObject payload = root["payload"].to<JsonObject>();
    WebSocketConnector<T>::_statefulService->read(payload, _stateReader);

    size_t len = measureJson(json);
    AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket.makeBuffer(len);
    if (buffer) {
      serializeJson(json, (char*)buffer->get(), len + 1);
      if (client) {
        client->text(buffer);
      } else {
        WebSocketConnector<T>::_webSocket.textAll(buffer);
      }
    }
  }
};

template <class T>
class WebSocketRx : virtual public WebSocketConnector<T> 
{
public:
  WebSocketRx(JsonStateUpdater<T> stateUpdater,
              StatefulService<T>* statefulService,
              AsyncWebServer* server,
              const char* webSocketPath,
              SecurityManager* securityManager,
              AuthenticationPredicate authenticationPredicate = AuthenticationPredicates::IS_ADMIN) 
    :
      WebSocketConnector<T>
      {
        statefulService,
        server,
        webSocketPath,
        securityManager,
        authenticationPredicate
      },
      _stateUpdater{stateUpdater} 
  {}

  WebSocketRx(JsonStateUpdater<T> stateUpdater,
              StatefulService<T>* statefulService,
              AsyncWebServer* server,
              const char* webSocketPath) 
    :
      WebSocketConnector<T>
      {
        statefulService, 
        server, 
        webSocketPath
      }, 
      _stateUpdater{stateUpdater}
  {}

 protected:
  virtual void onWSEvent(AsyncWebSocket* server,
                         AsyncWebSocketClient* client,
                         AwsEventType type,
                         void* arg,
                         uint8_t* data,
                         size_t len) 
  {
    if (type == WS_EVT_DATA) 
    {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len) 
      {
        if (info->opcode == WS_TEXT) 
        {
          JsonDocument json;
          DeserializationError error = deserializeJson(json, (char*)data);
          if (!error && json.is<JsonObject>()) {
            JsonObject jsonObject = json.as<JsonObject>();
            WebSocketConnector<T>::_statefulService->update(
                jsonObject, _stateUpdater, WebSocketConnector<T>::clientId(client));
          }
        }
      }
    }
  }

 private:
  JsonStateUpdater<T> _stateUpdater;
};

template <class T>
class WebSocketTxRx : public WebSocketTx<T>, public WebSocketRx<T> 
{
public:
  WebSocketTxRx(JsonStateReader<T> stateReader,
                JsonStateUpdater<T> stateUpdater,
                StatefulService<T>* statefulService,
                AsyncWebServer* server,
                const char* webSocketPath,
                SecurityManager* securityManager,
                AuthenticationPredicate authenticationPredicate = AuthenticationPredicates::IS_ADMIN) 
    :
      WebSocketConnector<T>
      {
        statefulService,
        server,
        webSocketPath,
        securityManager,
        authenticationPredicate
      },
      WebSocketTx<T>
      {
        stateReader,
        statefulService,
        server,
        webSocketPath,
        securityManager,
        authenticationPredicate
      },
      WebSocketRx<T>
      {
        stateUpdater,
        statefulService,
        server,
        webSocketPath,
        securityManager,
        authenticationPredicate
      } 
  {}

  WebSocketTxRx(JsonStateReader<T> stateReader,
                JsonStateUpdater<T> stateUpdater,
                StatefulService<T>* statefulService,
                AsyncWebServer* server,
                const char* webSocketPath) 
    :
      WebSocketConnector<T>{statefulService, server, webSocketPath},
      WebSocketTx<T>{stateReader, statefulService, server, webSocketPath},
      WebSocketRx<T>{stateUpdater, statefulService, server, webSocketPath} 
  {}

 protected:
  void onWSEvent(AsyncWebSocket* server,
                 AsyncWebSocketClient* client,
                 AwsEventType type,
                 void* arg,
                 uint8_t* data,
                 size_t len) 
  {
    WebSocketRx<T>::onWSEvent(server, client, type, arg, data, len);
    WebSocketTx<T>::onWSEvent(server, client, type, arg, data, len);
  }
};

#endif
