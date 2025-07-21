#ifndef WebSocketTxRx_h
#define WebSocketTxRx_h

#include <memory>
#include <StatefulService.h>
#include <ESPAsyncWebServer.h>
#include <SecurityManager.h>
#include <MemoryManager.h>

#define WEB_SOCKET_CLIENT_ID_MSG_SIZE 128

#define WEB_SOCKET_ORIGIN "websocket"
#define WEB_SOCKET_ORIGIN_CLIENT_ID_PREFIX "websocket:"

template <class T>
class WebSocketConnector 
{
protected:
  StatefulService<T>* _statefulService;
  AsyncWebServer* _server;
  std::unique_ptr<AsyncWebSocket> _webSocket;

  WebSocketConnector(StatefulService<T>* statefulService,
                     AsyncWebServer* server,
                     const char* webSocketPath,
                     SecurityManager* securityManager,
                     AuthenticationPredicate authenticationPredicate) 
    :
      _statefulService{statefulService}, 
      _server{server},
      _webSocket{new AsyncWebSocket(webSocketPath)}
  {
    _webSocket->setFilter(securityManager->filterRequest(authenticationPredicate));
    _webSocket->onEvent(std::bind(&WebSocketConnector::onWSEvent,
                                 this,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3,
                                 std::placeholders::_4,
                                 std::placeholders::_5,
                                 std::placeholders::_6));
    _server->addHandler(_webSocket.get());
    _server->on(webSocketPath, HTTP_GET, std::bind(&WebSocketConnector::forbidden, this, std::placeholders::_1));
  }

  WebSocketConnector(StatefulService<T>* statefulService,
                     AsyncWebServer* server,
                     const char* webSocketPath) 
    :
      _statefulService{statefulService}, 
      _server{server},
      _webSocket{new AsyncWebSocket(webSocketPath)}
  {
    _webSocket->onEvent(std::bind(&WebSocketConnector::onWSEvent,
                                 this,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3,
                                 std::placeholders::_4,
                                 std::placeholders::_5,
                                 std::placeholders::_6));
    _server->addHandler(_webSocket.get());
  }

  virtual void onWSEvent(AsyncWebSocket* server,
                         AsyncWebSocketClient* client,
                         AwsEventType type,
                         void* arg,
                         uint8_t* data,
                         size_t len) = 0;

  String clientId(AsyncWebSocketClient* client) 
  {
    return WEB_SOCKET_ORIGIN_CLIENT_ID_PREFIX + String(client->id());
  }

 private:
  void forbidden(AsyncWebServerRequest* request) 
  {
    request->send(403);
  }
};

// Delta-enabled WebSocket transmitter for optimized updates
template <class T>
class WebSocketTxDelta : virtual public WebSocketConnector<T> 
{
public:
  WebSocketTxDelta(JsonStateReader<T> stateReader,
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
      _stateReader{stateReader},
      _hasInitialState{false}
  {
    WebSocketConnector<T>::_statefulService->addUpdateHandler(
        [&](const String& originId) { transmitDelta(nullptr, originId); }, false);
  }

  WebSocketTxDelta(JsonStateReader<T> stateReader,
                   StatefulService<T>* statefulService,
                   AsyncWebServer* server,
                   const char* webSocketPath) 
    :
      WebSocketConnector<T>{statefulService, server, webSocketPath}, 
      _stateReader{stateReader},
      _hasInitialState{false}
  {
    WebSocketConnector<T>::_statefulService->addUpdateHandler(
      [&](const String& originId) { transmitDelta(nullptr, originId); }, false);
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
      transmitFullState(client, WEB_SOCKET_ORIGIN);
    }
  }

 private:
  JsonStateReader<T> _stateReader;
  bool _hasInitialState;
  String _lastStateSnapshot;

  void transmitId(AsyncWebSocketClient* client) 
  {
    JsonDocument json;
    JsonObject root = json.to<JsonObject>();
    root["type"] = "id";
    root["id"] = WebSocketConnector<T>::clientId(client).c_str();
    size_t len = measureJson(json);
    AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket->makeBuffer(len);
    if (buffer) {
      serializeJson(json, buffer->get(), len);
      client->text(buffer);
    }
  }

  // Send full state for new connections
  void transmitFullState(AsyncWebSocketClient* client, const String& originId) 
  {
    JsonDocument json;
    JsonObject root = json.to<JsonObject>();
    root["type"] = "payload";
    root["origin_id"] = originId;
    JsonObject payload = root["payload"].to<JsonObject>();
    WebSocketConnector<T>::_statefulService->read(payload, _stateReader);

    size_t len = measureJson(json);
    AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket->makeBuffer(len);
    if (buffer) {
      serializeJson(json, buffer->get(), len);
      if (client) {
        client->text(buffer);
      }
    }
    
    // Store snapshot for delta calculations
    _lastStateSnapshot = "";
    serializeJson(payload, _lastStateSnapshot);
    _hasInitialState = true;
  }

  // Send only changed fields (delta update)
  void transmitDelta(AsyncWebSocketClient* client, const String& originId) 
  {
    if (!_hasInitialState) {
      // First update - send full state
      transmitFullState(client, originId);
      return;
    }
    
    // Get current state
    JsonDocument currentJson;
    JsonObject currentState = currentJson.to<JsonObject>();
    WebSocketConnector<T>::_statefulService->read(currentState, _stateReader);
    
    // Serialize current state for comparison
    String currentStateStr = "";
    serializeJson(currentState, currentStateStr);
    
    // Check if anything changed
    if (currentStateStr == _lastStateSnapshot) {
      return; // No changes, don't send anything
    }
    
    // Parse previous state for comparison
    JsonDocument previousJson;
    deserializeJson(previousJson, _lastStateSnapshot);
    JsonObject previousState = previousJson.as<JsonObject>();
    
    // Build delta object with only changed fields
    JsonDocument deltaJson;
    JsonObject root = deltaJson.to<JsonObject>();
    root["type"] = "delta";
    root["origin_id"] = originId;
    JsonObject delta = root["delta"].to<JsonObject>();
    
    // Compare each field and add only changed ones
    for (JsonPair currentPair : currentState) {
      const char* key = currentPair.key().c_str();
      JsonVariant currentValue = currentPair.value();
      JsonVariant previousValue = previousState[key];
      
      if (currentValue != previousValue) {
        delta[key] = currentValue;
      }
    }
    
    // Send delta if there are changes
    if (delta.size() > 0) {
      size_t len = measureJson(deltaJson);
      AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket->makeBuffer(len);
      if (buffer) {
        serializeJson(deltaJson, buffer->get(), len);
        if (client) {
          client->text(buffer);
        } else {
          WebSocketConnector<T>::_webSocket->textAll(buffer);
        }
      }
      
      // Update snapshot
      _lastStateSnapshot = currentStateStr;
    }
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
        [&](const String& originId) { transmitData(nullptr, originId); }, false);
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
      [&](const String& originId) { transmitData(nullptr, originId); }, false);
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
    AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket->makeBuffer(len);
    if (buffer) {
      serializeJson(json, buffer->get(), len);
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
  void transmitData(AsyncWebSocketClient* client, const String& originId) 
  {
    JsonDocument json;
    JsonObject root = json.to<JsonObject>();
    root["type"] = "payload";
    root["origin_id"] = originId;
    //JsonObject payload = root.createNestedObject("payload");
    JsonObject payload = root["payload"].to<JsonObject>();
    WebSocketConnector<T>::_statefulService->read(payload, _stateReader);

    size_t len = measureJson(json);
    AsyncWebSocketMessageBuffer* buffer = WebSocketConnector<T>::_webSocket->makeBuffer(len);
    if (buffer) {
      serializeJson(json, buffer->get(), len);
      if (client) {
        client->text(buffer);
      } else {
        WebSocketConnector<T>::_webSocket->textAll(buffer);
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

// Delta functionality for optimized WebSocket communication
template <class T>
class WebSocketTxRxDelta : public WebSocketTxDelta<T>, public WebSocketRx<T> 
{
public:
  WebSocketTxRxDelta(JsonStateReader<T> stateReader,
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
      WebSocketTxDelta<T>
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

  WebSocketTxRxDelta(JsonStateReader<T> stateReader,
                     JsonStateUpdater<T> stateUpdater,
                     StatefulService<T>* statefulService,
                     AsyncWebServer* server,
                     const char* webSocketPath) 
    :
      WebSocketConnector<T>{statefulService, server, webSocketPath},
      WebSocketTxDelta<T>{stateReader, statefulService, server, webSocketPath},
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
    WebSocketTxDelta<T>::onWSEvent(server, client, type, arg, data, len);
  }
};

#endif
