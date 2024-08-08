// Can match simultaneous buttons, like, BUTTON_1 & BUTTON_2 = 0x03

#include "RemoteManager.h"
#include "codes.h"

RemoteManager::Buttons RemoteManager::getButtonsPressed(const BitVector* recorded) {
  if (recorded->get_nb_bits() != 52)
    return NONE;

  return static_cast<Buttons>(recorded->get_nth_byte(7));
}

RemoteManager::Buttons RemoteManager::getButtonsPressed(uint8_t* code, uint8_t size) {
  if (size != 7)
    return NONE;

  return static_cast<Buttons>(code[0]);
}

RemoteManager::Buttons RemoteManager::getButtonsPressed(RemotePacket& packet) {
  return static_cast<Buttons>(packet.button);
}

RemoteSerial RemoteManager::getSerial(const BitVector* recorded) {
  // beware, bitvector has bytes inverted!
  if (recorded->get_nb_bits() != 52)
    return {0};

  RemoteSerial serial;
  uint16_t hash = (recorded->get_nth_byte(4) << 8) | (recorded->get_nth_byte(3) & 0xff);
  uint16_t tblIndex = RemoteManager::getTableIndex(hash);
  uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + (tblIndex & 0xff)) ^ hash;

  serial.ser[0] = (recorded->get_nth_byte(5) ^ ki) & 0x0f;
  serial.ser[1] = recorded->get_nth_byte(2) ^ ki;
  serial.ser[2] = recorded->get_nth_byte(1) ^ ki;
  serial.ser[3] = recorded->get_nth_byte(0) ^ ki;

  return serial;
}

RemoteSerial RemoteManager::getSerial(const uint8_t* code, uint8_t size) {
  if (size != 7)
    return {0};  // it has to be precisely 7 bytes

  RemoteSerial serial;
  // Find codes index
  uint16_t hash = (code[2] << 8) | (code[3] & 0xff);
  uint16_t tblIndex = RemoteManager::getTableIndex(hash);
  uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + (tblIndex & 0xff)) ^ hash;

  serial.ser[0] = (code[1] ^ ki) & 0x0f;
  serial.ser[1] = code[4] ^ ki;
  serial.ser[2] = code[5] ^ ki;
  serial.ser[3] = code[6] ^ ki;

  return serial;
}

RemoteSerial RemoteManager::getSerial(RemotePacket& packet) {
  RemoteSerial serial;

  // Find codes index
  uint16_t tblIndex = RemoteManager::getTableIndex(packet.hash);
  uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + (tblIndex & 0xff)) ^ packet.hash;

  serial.ser[0] = (packet.enc1 ^ ki) & 0x0f;
  serial.ser[1] = packet.enc2 ^ ki;
  serial.ser[2] = packet.enc3 ^ ki;
  serial.ser[3] = packet.enc4 ^ ki;

  return serial;
}

bool RemoteManager::validateCode(const BitVector* recorded, RemoteSerial& serial) {
  RemoteSerial ser = RemoteManager::getSerial(recorded);

  if (ser == serial)
    return true;

  return false;
}

bool RemoteManager::validateCode(const uint8_t* code, uint8_t size, RemoteSerial& serial) {
  RemoteSerial ser = RemoteManager::getSerial(code, size);

  if (ser == serial)
    return true;

  return false;
}

bool RemoteManager::validateCode(RemotePacket& packet, RemoteSerial& serial) {
  RemoteSerial ser = RemoteManager::getSerial(packet);

  if (ser == serial)
    return true;

  return false;
}

uint16_t RemoteManager::getTableIndex(const uint16_t code) {
  for (uint16_t i = 0; i < sizeof(NICE_FLOR_S_TABLE_ENCODE); i++) {
    if (pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i) == code)
      return i;
  }

  return 0;
}
