#include "RFRemoteController.h"
#include <codes.h>

RfRemoteController* RfRemoteController::m_thisPtr = nullptr;

void callback_debug(const BitVector* recorded) 
{
  //char* pkt = recorded->to_str();
  //Serial.println(pkt);
  //free(pkt);

  if(RfRemoteController::getPtr())
  {
    RfRemoteController::getPtr()->processCode(recorded);
  }
}

RfRemoteController::RfRemoteController() : 
    m_rf{RF_PIN},
    m_lastHashReceived{0},
    m_lastHashCount{0},
    m_cb{0},
    m_cacheIndex{0}
{
    RfRemoteController::m_thisPtr = this;
    
    // Initialize cache
    for (int i = 0; i < TABLE_INDEX_CACHE_SIZE; i++) {
        m_cache[i].code = 0xFFFF;  // Invalid code
        m_cache[i].index = 0;
    }
}

void RfRemoteController::begin() {
  pinMode(RF_PIN, INPUT_PULLUP);

  // Needs tweaking
  m_rf.register_Receiver(RFMOD_TRIBIT, 18856, 1436, 1532, 0, 496, 980, 0, 0, 1448, 18856, 52, 
  callback_debug, 0);
  
  //m_rf.set_opt_wait_free_433(true, 100);
  //m_rf.set_inactivate_interrupts_handler_when_a_value_has_been_received(true);
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
            // Only process if we have valid data
            if(recorded->get_nb_bits() == 52) {
                RemotePacket packet = RfRemoteController::getPacket(recorded);
                
                // Quick validation before expensive table lookup
                if(packet.button > 0 && packet.hash > 0) {
                    RemoteSerial ser = RfRemoteController::getSerial(packet);

                    Serial.printf("Processing %04X - %s - %d\r\n", hash, ser.toString().c_str(), packet.button);
                    processAllCallbacks(packet, ser);
                }
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

void RfRemoteController::addCallback(RfRemoteControllerCallback cb)
{
  m_cb.push_back(cb);
}

RemotePacket RfRemoteController::getPacket(const BitVector *recorded)
{
    if(recorded->get_nb_bits() != 52)
        return {0};

    // Use direct byte access for better performance
    uint8_t byte0 = recorded->get_nth_byte(0);
    uint8_t byte1 = recorded->get_nth_byte(1);
    uint8_t byte2 = recorded->get_nth_byte(2);
    uint8_t byte3 = recorded->get_nth_byte(3);
    uint8_t byte4 = recorded->get_nth_byte(4);
    uint8_t byte5 = recorded->get_nth_byte(5);
    uint8_t byte6 = recorded->get_nth_byte(6);

    RemotePacket pack = {
        byte6,                                    // button
        static_cast<uint8_t>((byte5 >> 4) & 0x0f), // count
        static_cast<uint8_t>(byte5 & 0x0f),        // enc1
        static_cast<uint16_t>((byte4 << 8) | byte3), // hash
        byte2,                                    // enc2
        byte1,                                    // enc3
        byte0                                     // enc4
    };

    return pack;
}

void RfRemoteController::processAllCallbacks(RemotePacket packet, RemoteSerial serial)
{
  for(auto& cb : m_cb)
  {
    if(cb)
      cb(packet, serial);
  }
}

RfRemoteController::Buttons RfRemoteController::getButtonsPressed(const BitVector* recorded) {
  if (recorded->get_nb_bits() != 52)
    return NONE;

  return static_cast<Buttons>(recorded->get_nth_byte(7));
}

RfRemoteController::Buttons RfRemoteController::getButtonsPressed(uint8_t* code, uint8_t size) {
  if (size != 7)
    return NONE;

  return static_cast<Buttons>(code[0]);
}

RfRemoteController::Buttons RfRemoteController::getButtonsPressed(RemotePacket& packet) {
  return static_cast<Buttons>(packet.button);
}

RemoteSerial RfRemoteController::getSerial(const BitVector* recorded) {
  // beware, bitvector has bytes inverted!
  if (recorded->get_nb_bits() != 52)
    return {0};

  RemoteSerial serial;
  uint16_t hash = (recorded->get_nth_byte(4) << 8) | (recorded->get_nth_byte(3) & 0xff);
  uint16_t tblIndex = RfRemoteController::getTableIndexStatic(hash);
  uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + (tblIndex & 0xff)) ^ hash;

  serial.ser[0] = (recorded->get_nth_byte(5) ^ ki) & 0x0f;
  serial.ser[1] = recorded->get_nth_byte(2) ^ ki;
  serial.ser[2] = recorded->get_nth_byte(1) ^ ki;
  serial.ser[3] = recorded->get_nth_byte(0) ^ ki;

  return serial;
}

RemoteSerial RfRemoteController::getSerial(const uint8_t* code, uint8_t size) {
  if (size != 7)
    return {0};  // it has to be precisely 7 bytes

  RemoteSerial serial;
  // Find codes index
  uint16_t hash = (code[2] << 8) | (code[3] & 0xff);
  uint16_t tblIndex = RfRemoteController::getTableIndexStatic(hash);
  uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + (tblIndex & 0xff)) ^ hash;

  serial.ser[0] = (code[1] ^ ki) & 0x0f;
  serial.ser[1] = code[4] ^ ki;
  serial.ser[2] = code[5] ^ ki;
  serial.ser[3] = code[6] ^ ki;

  return serial;
}

RemoteSerial RfRemoteController::getSerial(RemotePacket& packet) {
  RemoteSerial serial;

  // Find codes index
  uint16_t tblIndex = RfRemoteController::getTableIndexStatic(packet.hash);
  uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + (tblIndex & 0xff)) ^ packet.hash;

  serial.ser[0] = (packet.enc1 ^ ki) & 0x0f;
  serial.ser[1] = packet.enc2 ^ ki;
  serial.ser[2] = packet.enc3 ^ ki;
  serial.ser[3] = packet.enc4 ^ ki;

  return serial;
}

bool RfRemoteController::validateCode(const BitVector* recorded, RemoteSerial& serial) {
  RemoteSerial ser = RfRemoteController::getSerial(recorded);

  if (ser == serial)
    return true;

  return false;
}

bool RfRemoteController::validateCode(const uint8_t* code, uint8_t size, RemoteSerial& serial) {
  RemoteSerial ser = RfRemoteController::getSerial(code, size);

  if (ser == serial)
    return true;

  return false;
}

bool RfRemoteController::validateCode(RemotePacket& packet, RemoteSerial& serial) {
  RemoteSerial ser = RfRemoteController::getSerial(packet);

  if (ser == serial)
    return true;

  return false;
}

uint16_t RfRemoteController::getTableIndex(const uint16_t code) {
  // First check cache
  for (uint8_t i = 0; i < TABLE_INDEX_CACHE_SIZE; i++) {
    if (m_cache[i].code == code) {
      return m_cache[i].index;
    }
  }
  
  // If not in cache, do optimized search
  uint16_t index = getTableIndexOptimized(code);
  
  // Add to cache
  m_cache[m_cacheIndex].code = code;
  m_cache[m_cacheIndex].index = index;
  m_cacheIndex = (m_cacheIndex + 1) % TABLE_INDEX_CACHE_SIZE;
  
  return index;
}

uint16_t RfRemoteController::getTableIndexStatic(const uint16_t code) {
  // Static wrapper that uses the instance pointer
  if (m_thisPtr) {
    return m_thisPtr->getTableIndex(code);
  }
  
  // Fallback to optimized search without cache
  return getTableIndexOptimized(code);
}

uint16_t RfRemoteController::getTableIndexOptimized(const uint16_t code) {
  // Since array is not sorted, use optimized linear search with early termination
  // Search in chunks to improve cache locality
  
  // First, check if it's a common code (first 1000 entries)
  for (uint16_t i = 0; i < 1000; i++) {
    if (pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i) == code) {
      return i;
    }
  }
  
  // Then check the rest in larger chunks
  for (uint16_t i = 1000; i < NICE_FLOR_S_TABLE_SIZE; i += 4) {
    // Check 4 entries at once for better performance
    if (pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i) == code) {
      return i;
    }
    if (i + 1 < NICE_FLOR_S_TABLE_SIZE && pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i + 1) == code) {
      return i + 1;
    }
    if (i + 2 < NICE_FLOR_S_TABLE_SIZE && pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i + 2) == code) {
      return i + 2;
    }
    if (i + 3 < NICE_FLOR_S_TABLE_SIZE && pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i + 3) == code) {
      return i + 3;
    }
  }

  return 0;
}