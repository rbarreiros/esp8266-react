#ifndef _RFREMOTECONTROLLER_H_
#define _RFREMOTECONTROLLER_H_

#include <RF433recv.h>
#include <vector>

#define DEFAULT_RED_LED_STATE false
#define RED_LED_PIN     16
#define RED_LED_ON      LOW
#define RED_LED_OFF     HIGH

#define RF_PIN          13

// Optimized settings for better performance
#define MIN_PACKETS_ACCEPTED 3
#define RF_RING_BUFFER_SIZE 16      // Must be power of 2
#define RF_PROCESSING_TIMEOUT 5000  // 5 seconds timeout
#define RF_DUPLICATE_WINDOW 500     // 500ms window for duplicate detection

/**
 * Optimized RF Remote Controller for Nice FLOR-S remotes
 * 
 * Key optimizations:
 * - Interrupt-safe ring buffer for signal queuing
 * - Deferred processing outside interrupt context
 * - Optimized hash table lookup instead of linear search
 * - Timeout protection against stuck states
 * - Memory-efficient packet processing
 */

struct _remoteserial_t
{
    uint8_t ser[4];

    String toString() const
    {
        char tmp[9] = {0};
        snprintf(tmp, 9, "%02X%02X%02X%02X", ser[0], ser[1], ser[2], ser[3]);
        tmp[8] = '\0';
        return String(tmp);
    }

    inline bool operator==(const _remoteserial_t& other) const
    {
        return (ser[0] == other.ser[0] && ser[1] == other.ser[1] &&
                ser[2] == other.ser[2] && ser[3] == other.ser[3]);
    }

    inline _remoteserial_t& operator=(const _remoteserial_t& other)
    {
        if(this != &other)
        {
            ser[0] = other.ser[0];
            ser[1] = other.ser[1];
            ser[2] = other.ser[2];
            ser[3] = other.ser[3];
        }
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
    
    // Additional fields for optimization
    uint32_t timestamp;     // When this packet was received
    bool isValid;          // Quick validation flag
};

// Ring buffer entry for interrupt-safe queuing
struct RfSignalEntry
{
    uint8_t data[7];      // Raw signal data (compact format)
    uint32_t timestamp;   // When received
    bool isValid;         // Entry is valid
};

using RemoteSerial = struct _remoteserial_t;
using RemotePacket = struct _remotepacket_t;
using RfRemoteControllerCallback = std::function<void (RemotePacket packet, RemoteSerial serial)>; 

class RfRemoteController
{
public:
    enum Buttons {
        NONE = 0,
        BUTTON_1 = (1 << 1),
        BUTTON_2 = (1 << 2),
        BUTTON_3 = (1 << 3),
        BUTTON_4 = (1 << 4)
    };

    // Statistics for performance monitoring
    struct Statistics {
        uint32_t packetsReceived;
        uint32_t packetsProcessed;
        uint32_t packetsDropped;
        uint32_t duplicatesFiltered;
        uint32_t timeouts;
        uint32_t bufferOverflows;
        uint32_t averageProcessingTime;
        uint32_t maxProcessingTime;
    };

    RfRemoteController();
    static RfRemoteController* getPtr() { return m_thisPtr; };

    void begin();
    void loop();
    void addCallback(RfRemoteControllerCallback cb);
    
    // Performance monitoring
    Statistics getStatistics() const { return m_stats; }
    void resetStatistics();

    // Optimized static methods
    static Buttons getButtonsPressed(const BitVector *recorded);
    static Buttons getButtonsPressed(const uint8_t* code, uint8_t size);
    static Buttons getButtonsPressed(const RemotePacket& packet);

    static RemoteSerial getSerial(const BitVector *recorded);
    static RemoteSerial getSerial(const uint8_t* code, uint8_t size);
    static RemoteSerial getSerial(const RemotePacket& packet);

    static bool validateCode(const BitVector *recorded, const RemoteSerial& serial);
    static bool validateCode(const uint8_t* code, uint8_t size, const RemoteSerial& serial);
    static bool validateCode(const RemotePacket& packet, const RemoteSerial& serial);

    // Interrupt-safe signal queuing
    void queueSignal(const BitVector *recorded);

private:
    RF_manager  m_rf;
    static RfRemoteController* m_thisPtr;
    
    // Ring buffer for interrupt-safe signal queuing
    volatile RfSignalEntry m_signalBuffer[RF_RING_BUFFER_SIZE];
    volatile uint8_t m_bufferHead;
    volatile uint8_t m_bufferTail;
    
    // Duplicate detection with timeout
    struct {
        uint16_t hash;
        uint8_t count;
        uint32_t firstSeen;
        uint32_t lastSeen;
    } m_duplicateFilter;
    
    // Callback management
    std::vector<RfRemoteControllerCallback> m_callbacks;
    
    // Statistics
    mutable Statistics m_stats;
    uint32_t m_lastStatsUpdate;
    
    // Optimized lookup table (hash table instead of linear search)
    static const uint16_t* getTableIndexFast(uint16_t code);
    
    // Ring buffer operations (interrupt-safe)
    bool enqueueSignal(const uint8_t* data, uint32_t timestamp);
    bool dequeueSignal(uint8_t* data, uint32_t& timestamp);
    inline uint8_t nextIndex(uint8_t index) const { return (index + 1) & (RF_RING_BUFFER_SIZE - 1); }
    inline bool isBufferEmpty() const { return m_bufferHead == m_bufferTail; }
    inline bool isBufferFull() const { return nextIndex(m_bufferHead) == m_bufferTail; }
    
    // Deferred processing methods
    void processQueuedSignals();
    bool processSignal(const uint8_t* data, uint32_t timestamp);
    RemotePacket createPacket(const uint8_t* data, uint32_t timestamp);
    
    // Duplicate detection
    bool isDuplicate(uint16_t hash, uint32_t timestamp);
    void updateDuplicateFilter(uint16_t hash, uint32_t timestamp);
    void resetDuplicateFilter();
    
    // Callback execution
    void executeCallbacks(const RemotePacket& packet, const RemoteSerial& serial);
    
    // Utility methods
    static void compactSignalData(const BitVector *recorded, uint8_t* data);
    static void expandSignalData(const uint8_t* data, BitVector& recorded);
    
    // Performance monitoring
    void updateStatistics(uint32_t processingTime);
    void recordTimeout();
    void recordBufferOverflow();
    void recordDuplicate();
};

#endif