#ifndef _RFREMOTECONTROLLER_H_
#define _RFREMOTECONTROLLER_H_

#include <RCSwitch.h>
#include <vector>

#define DEFAULT_RED_LED_STATE false
#define RED_LED_PIN     16
#define RED_LED_ON      LOW
#define RED_LED_OFF     HIGH


#define RF_PIN          15

struct Remote
{
    String description;
    unsigned int proto;
    unsigned long value;
};

class RfRemoteController
{
public:
    RfRemoteController();
    void begin();
    void loop();

private:
    RCSwitch m_rf;

};




#endif