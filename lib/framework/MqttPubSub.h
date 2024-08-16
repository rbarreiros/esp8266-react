#ifndef MqttPubSub_h
#define MqttPubSub_h

#include <StatefulService.h>
#include <espMqttClientAsync.h>

#define MQTT_ORIGIN_ID "mqtt"

template <class T>
class MqttConnector 
{
protected:
  StatefulService<T>* _statefulService;
  espMqttClientAsync* _mqttClient;

  MqttConnector(StatefulService<T>* statefulService, espMqttClientAsync* mqttClient) 
    :
      _statefulService{statefulService}, 
      _mqttClient{mqttClient} 
  {
    _mqttClient->onConnect(std::bind(&MqttConnector::onConnect, this));
  }

  virtual void onConnect() = 0;

public:
  inline espMqttClientAsync* getMqttClient() const 
  {
    return _mqttClient;
  }
};

template <class T>
class MqttPub : virtual public MqttConnector<T> 
{
public:
  MqttPub(JsonStateReader<T> stateReader,
          StatefulService<T>* statefulService,
          espMqttClientAsync* mqttClient,
          const String& pubTopic = "",
          bool retain = false) 
    :
      MqttConnector<T>{statefulService, mqttClient},
      _stateReader{stateReader},
      _pubTopic{pubTopic},
      _retain{retain} 
  {
    MqttConnector<T>::_statefulService->addUpdateHandler([&](const String& originId) { publish(); }, false);
  }

  void setRetain(const bool retain) 
  {
    _retain = retain;
    publish();
  }

  void setPubTopic(const String& pubTopic) 
  {
    _pubTopic = pubTopic;
    publish();
  }

 protected:
  virtual void onConnect() 
  {
    publish();
  }

 private:
  JsonStateReader<T> _stateReader;
  String _pubTopic;
  bool _retain;

  void publish() 
  {
    if (_pubTopic.length() > 0 && MqttConnector<T>::_mqttClient->connected()) 
    {
      // serialize to json doc
      JsonDocument json;
      JsonObject jsonObject = json.to<JsonObject>();
      MqttConnector<T>::_statefulService->read(jsonObject, _stateReader);

      // serialize to string
      String payload;
      serializeJson(json, payload);

      // publish the payload
      MqttConnector<T>::_mqttClient->publish(_pubTopic.c_str(), 0, _retain, payload.c_str());
    }
  }
};

template <class T>
class MqttSub : virtual public MqttConnector<T> 
{
public:
  MqttSub(JsonStateUpdater<T> stateUpdater,
          StatefulService<T>* statefulService,
          espMqttClientAsync* mqttClient,
          const String& subTopic = "") 
    :
      MqttConnector<T>{statefulService, mqttClient}, 
      _stateUpdater{stateUpdater}, 
      _subTopic{subTopic} 
  {
    MqttConnector<T>::_mqttClient->onMessage(std::bind(&MqttSub::onMqttMessage,
                                                       this,
                                                       std::placeholders::_1,
                                                       std::placeholders::_2,
                                                       std::placeholders::_3,
                                                       std::placeholders::_4,
                                                       std::placeholders::_5,
                                                       std::placeholders::_6));
  }

  void setSubTopic(const String& subTopic) 
  {
    Serial.printf("Attempting to set topic: %s\r\n", subTopic.c_str());
    if (!_subTopic.equals(subTopic)) 
    {
      // unsubscribe from the existing topic if one was set
      if (_subTopic.length() > 0) {
        MqttConnector<T>::_mqttClient->unsubscribe(_subTopic.c_str());
      }
      // set the new topic and re-configure the subscription
      _subTopic = subTopic;
      Serial.printf("Setting subtopic %s - %p\r\n", _subTopic.c_str(), &_subTopic);
      subscribe();
    }
  }

 protected:
  virtual void onConnect() 
  {
    subscribe();
  }

 private:
  JsonStateUpdater<T> _stateUpdater;
  String _subTopic;

  void subscribe() 
  {
    if (_subTopic.length() > 0) {
      Serial.printf("Subscribing to topic %s\r\n", _subTopic.c_str());
      if(MqttConnector<T>::_mqttClient->subscribe(_subTopic.c_str(), 2))
        Serial.println("With success.");
      else
        Serial.println("With failure.");

      Serial.printf("Subscribed to subtopic: %s <--- %p\r\n", _subTopic.c_str(), &_subTopic);
    }
  }

  void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, 
                     const char* topic, 
                     const uint8_t* payload, 
                     size_t len, 
                     size_t index, 
                     size_t total) 
  {
    Serial.printf("Received mqtt message from %s with payload %s our subtopic is %s (%p)\r\n", topic, reinterpret_cast<const char*>(payload), _subTopic.c_str(), &_subTopic); 
    // we only care about the topic we are watching in this class
    if (strcmp(_subTopic.c_str(), topic)) {
      Serial.printf("Not equal?!?! %s and %s\r\n", topic, _subTopic.c_str());
      return;
    }

    // deserialize from string
    JsonDocument json;
    DeserializationError error = deserializeJson(json, payload, len);
    if (!error && json.is<JsonObject>()) {
      JsonObject jsonObject = json.as<JsonObject>();
      MqttConnector<T>::_statefulService->update(jsonObject, _stateUpdater, MQTT_ORIGIN_ID);
    }
    else 
    {
      Serial.printf("Error deserealizing !?!?!?! %s \r\n", error.c_str());
    }
  }
};

template <class T>
class MqttPubSub : public MqttPub<T>, public MqttSub<T> 
{
public:
  MqttPubSub(JsonStateReader<T> stateReader,
             JsonStateUpdater<T> stateUpdater,
             StatefulService<T>* statefulService,
             espMqttClientAsync* mqttClient,
             const String& pubTopic = "",
             const String& subTopic = "",
             bool retain = false) 
    :
      MqttConnector<T>{statefulService, mqttClient},
      MqttPub<T>{stateReader, statefulService, mqttClient, pubTopic, retain},
      MqttSub<T>{stateUpdater, statefulService, mqttClient, subTopic} 
  {
  }

 public:
  void configureTopics(const String& pubTopic, const String& subTopic) 
  {
    MqttSub<T>::setSubTopic(subTopic);
    MqttPub<T>::setPubTopic(pubTopic);
  }

 protected:
  void onConnect() 
  {
    MqttSub<T>::onConnect();
    MqttPub<T>::onConnect();
  }
};

#endif  // end MqttPubSub
