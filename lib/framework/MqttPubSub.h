#ifndef MqttPubSub_h
#define MqttPubSub_h

#include <StatefulService.h>
#include <espMqttClientAsync.h>

#define MQTT_ORIGIN_ID "mqtt"
#define TOPIC_BUFFER_SIZE 128

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
          const char* pubTopic = "",
          bool retain = false) 
    :
      MqttConnector<T>{statefulService, mqttClient},
      _stateReader{stateReader},
      //_pubTopic{pubTopic},
      _retain{retain} 
  {
    strncpy(_pubTopic, pubTopic, TOPIC_BUFFER_SIZE);
    MqttConnector<T>::_statefulService->addUpdateHandler([&](const char* originId) { publish(); }, false);
  }

  void setRetain(const bool retain) 
  {
    _retain = retain;
    publish();
  }

  void setPubTopic(const char* pubTopic) 
  {
    //_pubTopic = pubTopic;
    strncpy(_pubTopic, pubTopic, TOPIC_BUFFER_SIZE);
    publish();
  }

 protected:
  virtual void onConnect() 
  {
    publish();
  }

 private:
  JsonStateReader<T> _stateReader;
  char _pubTopic[TOPIC_BUFFER_SIZE];
  bool _retain;

  void publish() 
  {
    if (strlen(_pubTopic) > 0 && MqttConnector<T>::_mqttClient->connected()) 
    {
      // serialize to json doc
      JsonDocument json;
      JsonObject jsonObject = json.to<JsonObject>();
      MqttConnector<T>::_statefulService->read(jsonObject, _stateReader);

      // serialize to string
      size_t len = measureJson(json);
      char buff[len + 1];
      serializeJson(json, buff, len + 1);

      // publish the payload
      MqttConnector<T>::_mqttClient->publish(_pubTopic, 0, _retain, buff);
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
          const char* subTopic = "") 
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

  void setSubTopic(const char* subTopic) 
  {
    if(strcmp(_subTopic, subTopic) != 0)
    {
      if(strlen(_subTopic) > 0)
        MqttConnector<T>::_mqttClient->unsubscribe(_subTopic);

      strncpy(_subTopic, subTopic, TOPIC_BUFFER_SIZE);
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
  char _subTopic[TOPIC_BUFFER_SIZE];

  void subscribe() 
  {
    if (strlen(_subTopic) > 0) {
      MqttConnector<T>::_mqttClient->subscribe(_subTopic, 2);
    }
  }

  void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, 
                     const char* topic, 
                     const uint8_t* payload, 
                     size_t len, 
                     size_t index, 
                     size_t total) 
  {
    // we only care about the topic we are watching in this class
    if (strcmp(_subTopic, topic)) {
      return;
    }

    // deserialize from string
    JsonDocument json;
    DeserializationError error = deserializeJson(json, payload, len);
    if (!error && json.is<JsonObject>()) {
      JsonObject jsonObject = json.as<JsonObject>();
      MqttConnector<T>::_statefulService->update(jsonObject, _stateUpdater, MQTT_ORIGIN_ID);
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
             const char* pubTopic = "",
             const char* subTopic = "",
             bool retain = false) 
    :
      MqttConnector<T>{statefulService, mqttClient},
      MqttPub<T>{stateReader, statefulService, mqttClient, pubTopic, retain},
      MqttSub<T>{stateUpdater, statefulService, mqttClient, subTopic} 
  {
  }

 public:
  void configureTopics(const char* pubTopic, const char* subTopic) 
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
