#include "RFRemoteController.h"

RfRemoteController* RfRemoteController::m_thisPtr = nullptr;

void callback_debug(const BitVector* recorded) {
  // Serial.print(F("Code received: "));
  //char* printed_code = recorded->to_str();

    /*
  if (printed_code) {
    // Serial.print(recorded->get_nb_bits());
    // Serial.print(F(" bits: ["));
    Serial.println(printed_code);
    // Serial.print(F("]\n"));

    free(printed_code);
  }
  */
    RfRemoteController::getPtr()->processCode(recorded);
}

RfRemoteController::RfRemoteController() : 
    m_rf{RF_PIN},
    m_lastHashReceived{0},
    m_lastHashCount{0},
    m_cb{nullptr}
{
    RfRemoteController::m_thisPtr = this;
}

void RfRemoteController::begin() {
  pinMode(RF_PIN, INPUT);

  // Needs tweaking
  m_rf.register_Receiver(RFMOD_TRIBIT, 18856, 1436, 1532, 0, 496, 980, 0, 0, 1448, 18856, 52, 
    callback_debug, 1);

  // rf.set_opt_wait_free_433(true, 10);
  m_rf.set_inactivate_interrupts_handler_when_a_value_has_been_received(true);
  m_rf.activate_interrupts_handler();
}

void RfRemoteController::loop() {
  m_rf.do_events();
}

void RfRemoteController::processCode(const BitVector* recorded)
{
    uint16_t hash = (recorded->get_nth_byte(2) << 8) | (recorded->get_nth_byte(3) & 0xff);

    if(m_lastHashReceived == hash)
    {
        ++m_lastHashCount;

        if(m_lastHashCount == MIN_PACKETS_ACCEPTED)
        {
            Serial.println("Processing");

            RemotePacket packet = getPacket(recorded);
            RemoteSerial ser = RemoteManager::getSerial(recorded);
            
            if(packet.button > 0 && packet.hash > 0)
            {
                if(m_cb)
                    m_cb(packet, ser);
            }

            m_lastHashCount = 0;
            m_lastHashReceived = 0;
        }
    }
    else 
    {
        m_lastHashCount = 1;
        m_lastHashReceived = hash;
    }
}

RemotePacket RfRemoteController::getPacket(const BitVector *recorded)
{
    if(recorded->get_nb_bits() != 52)
        return {0};

    RemotePacket pack = {
        recorded->get_nth_byte(6),
        (recorded->get_nth_byte(5) >> 4) & 0x0f,
        (recorded->get_nth_byte(5) & 0x0f),
        (recorded->get_nth_byte(4) << 8) | (recorded->get_nth_byte(3) & 0xff),
        recorded->get_nth_byte(2),
        recorded->get_nth_byte(1),
        recorded->get_nth_byte(0)
    };

    return pack;
}