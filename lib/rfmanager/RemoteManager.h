#ifndef _REMOTEMANAGER_H_
#define _REMOTEMANAGER_H_

#include <stdint.h>
#include "RF433recv.h"
#include "codes.h"

/**
 * We're using Nice FLOR-S remotes, only because I saw them in aliexpress and they were
 * pretty and small (Remotes are Nice INTI2, which apparently use a modified Nice FLOR-S)
 * 
 * The only thing that changes apparently, is the counter formula, which works for
 * button 1, but button 2 doesnt (most likely a power of, or multiplication, since 
 * using button 1 works), for now, it does ignore the counter to validate the remote
 * 
 * The only thing this class does is, extract serial from code, and validate
 * a code against a serial.
 * 
 */

struct _remoteserial_t
{
    uint8_t ser[4];

    inline bool operator==(const _remoteserial_t& other)
    {
        if(ser[0] == other.ser[0] && ser[1] == other.ser[1] &&
            ser[2] == other.ser[2] && ser[3] == other.ser[3])
            return true;

        return false;
    }

    inline _remoteserial_t operator=(const _remoteserial_t& other)
    {
        if(this == &other)
            return *this;

        ser[0] = other.ser[0];
        ser[1] = other.ser[1];
        ser[2] = other.ser[2];
        ser[3] = other.ser[3];

        return *this;
    }

};

struct _remotepacket_t
{
    uint8_t button;
    uint8_t count:4;
    uint8_t enc1:4;
    uint16_t hash;
    uint8_t enc2;
    uint8_t enc3;
    uint8_t enc4;
};

using RemoteSerial = struct _remoteserial_t;
using RemotePacket = struct _remotepacket_t;

class RemoteManager
{
public:

    enum Buttons {
        NONE = 0,
        BUTTON_1 = (1 << 1),
        BUTTON_2 = (1 << 2),
        BUTTON_3 = (1 << 3),
        BUTTON_4 = (1 << 4)
    };

    // Can match simultaneous buttons, like, BUTTON_1 & BUTTON_2 = 0x03
    static Buttons getButtonsPressed(const BitVector *recorded);
    static Buttons getButtonsPressed(uint8_t* code, uint8_t size);
    static Buttons getButtonsPressed(RemotePacket& packet);

    static RemoteSerial getSerial(const BitVector *recorded);
    static RemoteSerial getSerial(const uint8_t* code, uint8_t size);
    static RemoteSerial getSerial(RemotePacket &packet);

    static bool validateCode(const BitVector *recorded, RemoteSerial& serial);
    static bool validateCode(const uint8_t* code, uint8_t size, RemoteSerial& serial);
    static bool validateCode(RemotePacket& packet, RemoteSerial& serial);

private:
    static uint16_t getTableIndex(const uint16_t code);
};

#endif