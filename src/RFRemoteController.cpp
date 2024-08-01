#include "RFRemoteController.h"

RfRemoteController::RfRemoteController()
    :
        m_rf{}
{
  m_rf.enableReceive(RF_PIN);
}



void RfRemoteController::begin()
{
    
}

void RfRemoteController::loop()
{
    if(m_rf.available())
    {
        Serial.printf("Received value %ld\r\n", m_rf.getReceivedValue());
        m_rf.resetAvailable();
    }
}
