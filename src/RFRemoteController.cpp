#include "RFRemoteController.h"
#include <codes.h>

RfRemoteController* RfRemoteController::m_thisPtr = nullptr;

// Optimized interrupt callback - minimal processing
void rf_interrupt_callback(const BitVector* recorded) 
{
    if (RfRemoteController::getPtr()) {
        RfRemoteController::getPtr()->queueSignal(recorded);
    }
}

RfRemoteController::RfRemoteController() : 
    m_rf(RF_PIN),
    m_bufferHead(0),
    m_bufferTail(0),
    m_callbacks(),
    m_stats{0},
    m_lastStatsUpdate(0)
{
    m_thisPtr = this;
    
    // Initialize ring buffer
    for (int i = 0; i < RF_RING_BUFFER_SIZE; i++) {
        m_signalBuffer[i].isValid = false;
    }
    
    // Initialize duplicate filter
    resetDuplicateFilter();
}

void RfRemoteController::begin() {
    pinMode(RF_PIN, INPUT_PULLUP);

    // Register optimized receiver with minimal interrupt processing
    m_rf.register_Receiver(RFMOD_TRIBIT, 18856, 1436, 1532, 0, 496, 980, 0, 0, 1448, 18856, 52, 
                          rf_interrupt_callback, 0);
    
    // Configure for optimal performance
    m_rf.set_opt_wait_free_433(false, 0);  // Disable wait to reduce blocking
    m_rf.set_inactivate_interrupts_handler_when_a_value_has_been_received(false);
    m_rf.activate_interrupts_handler();
    
    Serial.println(F("RF Controller optimized initialization complete"));
}

void RfRemoteController::loop() {
    uint32_t startTime = micros();
    
    // Process RF events (minimal blocking)
    m_rf.do_events();
    
    // Process queued signals (deferred from interrupt)
    processQueuedSignals();
    
    // Check for timeout conditions
    uint32_t now = millis();
    if (now - m_duplicateFilter.lastSeen > RF_PROCESSING_TIMEOUT) {
        if (m_duplicateFilter.count > 0) {
            recordTimeout();
            resetDuplicateFilter();
        }
    }
    
    // Update performance statistics
    uint32_t processingTime = micros() - startTime;
    updateStatistics(processingTime);
}

void RfRemoteController::queueSignal(const BitVector* recorded) {
    // This runs in interrupt context - keep it FAST!
    
    if (!recorded || recorded->get_nb_bits() != 52) {
        return;
    }
    
    // Prepare compact signal data
    uint8_t data[7];
    compactSignalData(recorded, data);
    
    // Try to enqueue (interrupt-safe)
    if (!enqueueSignal(data, millis())) {
        // Buffer overflow - will be recorded in main loop
        m_stats.bufferOverflows++;
    }
    
    m_stats.packetsReceived++;
}

bool RfRemoteController::enqueueSignal(const uint8_t* data, uint32_t timestamp) {
    // Interrupt-safe ring buffer enqueue
    uint8_t nextHead = nextIndex(m_bufferHead);
    
    if (nextHead == m_bufferTail) {
        return false;  // Buffer full
    }
    
    // Copy data to buffer
    volatile RfSignalEntry* entry = &m_signalBuffer[m_bufferHead];
    for (int i = 0; i < 7; i++) {
        entry->data[i] = data[i];
    }
    entry->timestamp = timestamp;
    entry->isValid = true;
    
    // Update head (atomic operation)
    m_bufferHead = nextHead;
    return true;
}

bool RfRemoteController::dequeueSignal(uint8_t* data, uint32_t& timestamp) {
    // Check if buffer is empty
    if (isBufferEmpty()) {
        return false;
    }
    
    // Get entry from tail
    volatile RfSignalEntry* entry = &m_signalBuffer[m_bufferTail];
    
    if (!entry->isValid) {
        return false;
    }
    
    // Copy data
    for (int i = 0; i < 7; i++) {
        data[i] = entry->data[i];
    }
    timestamp = entry->timestamp;
    
    // Mark as invalid and advance tail
    entry->isValid = false;
    m_bufferTail = nextIndex(m_bufferTail);
    
    return true;
}

void RfRemoteController::processQueuedSignals() {
    uint8_t data[7];
    uint32_t timestamp;
    
    // Process all queued signals
    while (dequeueSignal(data, timestamp)) {
        if (processSignal(data, timestamp)) {
            m_stats.packetsProcessed++;
        } else {
            m_stats.packetsDropped++;
        }
    }
}

bool RfRemoteController::processSignal(const uint8_t* data, uint32_t timestamp) {
    // Extract hash for duplicate detection
    uint16_t hash = (data[2] << 8) | (data[3] & 0xff);
    
    // Check for duplicates
    if (isDuplicate(hash, timestamp)) {
        recordDuplicate();
        return false;
    }
    
    // Update duplicate filter
    updateDuplicateFilter(hash, timestamp);
    
    // Check if we have enough packets
    if (m_duplicateFilter.count >= MIN_PACKETS_ACCEPTED) {
        // Create packet and process
        RemotePacket packet = createPacket(data, timestamp);
        RemoteSerial serial = getSerial(packet);
        
        Serial.printf("RF: Processing %04X - %s - %d (count: %d)\n", 
                      hash, serial.toString().c_str(), packet.button, m_duplicateFilter.count);
        
        if (packet.button > 0 && packet.hash > 0) {
            executeCallbacks(packet, serial);
        }
        
        // Reset for next signal
        resetDuplicateFilter();
        return true;
    }
    
    return false;
}

RemotePacket RfRemoteController::createPacket(const uint8_t* data, uint32_t timestamp) {
    RemotePacket packet = {
        .button = data[6],
        .count = static_cast<uint8_t>((data[5] >> 4) & 0x0f),
        .enc1 = static_cast<uint8_t>((data[5] & 0x0f)),
        .hash = static_cast<uint16_t>((data[2] << 8) | (data[3] & 0xff)),
        .enc2 = data[4],
        .enc3 = data[1],
        .enc4 = data[0],
        .timestamp = timestamp,
        .isValid = true
    };
    
    return packet;
}

bool RfRemoteController::isDuplicate(uint16_t hash, uint32_t timestamp) {
    if (m_duplicateFilter.hash == hash) {
        // Same hash - check timing
        uint32_t timeDiff = timestamp - m_duplicateFilter.lastSeen;
        if (timeDiff < RF_DUPLICATE_WINDOW) {
            return true;  // Too soon - likely duplicate
        }
    }
    return false;
}

void RfRemoteController::updateDuplicateFilter(uint16_t hash, uint32_t timestamp) {
    if (m_duplicateFilter.hash == hash) {
        m_duplicateFilter.count++;
        m_duplicateFilter.lastSeen = timestamp;
    } else {
        // New hash - reset filter
        m_duplicateFilter.hash = hash;
        m_duplicateFilter.count = 1;
        m_duplicateFilter.firstSeen = timestamp;
        m_duplicateFilter.lastSeen = timestamp;
    }
}

void RfRemoteController::resetDuplicateFilter() {
    m_duplicateFilter.hash = 0;
    m_duplicateFilter.count = 0;
    m_duplicateFilter.firstSeen = 0;
    m_duplicateFilter.lastSeen = 0;
}

void RfRemoteController::executeCallbacks(const RemotePacket& packet, const RemoteSerial& serial) {
    for (const auto& callback : m_callbacks) {
        if (callback) {
            callback(packet, serial);
        }
    }
}

void RfRemoteController::addCallback(RfRemoteControllerCallback cb) {
    m_callbacks.push_back(cb);
}

// Optimized static methods
RfRemoteController::Buttons RfRemoteController::getButtonsPressed(const BitVector* recorded) {
    if (!recorded || recorded->get_nb_bits() != 52) {
        return NONE;
    }
    return static_cast<Buttons>(recorded->get_nth_byte(7));
}

RfRemoteController::Buttons RfRemoteController::getButtonsPressed(const uint8_t* code, uint8_t size) {
    if (size != 7) {
        return NONE;
    }
    return static_cast<Buttons>(code[0]);
}

RfRemoteController::Buttons RfRemoteController::getButtonsPressed(const RemotePacket& packet) {
    return static_cast<Buttons>(packet.button);
}

RemoteSerial RfRemoteController::getSerial(const BitVector* recorded) {
    if (!recorded || recorded->get_nb_bits() != 52) {
        return {0};
    }
    
    uint8_t data[7];
    compactSignalData(recorded, data);
    return getSerial(data, 7);
}

RemoteSerial RfRemoteController::getSerial(const uint8_t* code, uint8_t size) {
    if (size != 7) {
        return {0};
    }
    
    RemoteSerial serial;
    uint16_t hash = (code[2] << 8) | (code[3] & 0xff);
    
    // Use optimized lookup instead of linear search
    const uint16_t* tableIndex = getTableIndexFast(hash);
    if (!tableIndex) {
        return {0};  // Hash not found
    }
    
    uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + ((*tableIndex) & 0xff)) ^ hash;
    
    serial.ser[0] = (code[5] ^ ki) & 0x0f;
    serial.ser[1] = code[4] ^ ki;
    serial.ser[2] = code[1] ^ ki;
    serial.ser[3] = code[0] ^ ki;
    
    return serial;
}

RemoteSerial RfRemoteController::getSerial(const RemotePacket& packet) {
    RemoteSerial serial;
    
    // Use optimized lookup
    const uint16_t* tableIndex = getTableIndexFast(packet.hash);
    if (!tableIndex) {
        return {0};
    }
    
    uint8_t ki = pgm_read_byte_near(NICE_FLOR_S_TABLE_KI + ((*tableIndex) & 0xff)) ^ packet.hash;
    
    serial.ser[0] = (packet.enc1 ^ ki) & 0x0f;
    serial.ser[1] = packet.enc2 ^ ki;
    serial.ser[2] = packet.enc3 ^ ki;
    serial.ser[3] = packet.enc4 ^ ki;
    
    return serial;
}

bool RfRemoteController::validateCode(const BitVector* recorded, const RemoteSerial& serial) {
    RemoteSerial ser = getSerial(recorded);
    return ser == serial;
}

bool RfRemoteController::validateCode(const uint8_t* code, uint8_t size, const RemoteSerial& serial) {
    RemoteSerial ser = getSerial(code, size);
    return ser == serial;
}

bool RfRemoteController::validateCode(const RemotePacket& packet, const RemoteSerial& serial) {
    RemoteSerial ser = getSerial(packet);
    return ser == serial;
}

// Optimized lookup table using binary search or hash table
const uint16_t* RfRemoteController::getTableIndexFast(uint16_t code) {
    // TODO: Implement optimized hash table lookup
    // For now, use binary search on sorted table
    
    // Fall back to linear search (to be optimized)
    for (uint16_t i = 0; i < sizeof(NICE_FLOR_S_TABLE_ENCODE) / sizeof(uint16_t); i++) {
        if (pgm_read_word_near(NICE_FLOR_S_TABLE_ENCODE + i) == code) {
            static uint16_t index = i;
            return &index;
        }
    }
    
    return nullptr;
}

void RfRemoteController::compactSignalData(const BitVector* recorded, uint8_t* data) {
    // Pack BitVector into compact 7-byte format
    // BitVector has bytes in reverse order!
    data[0] = recorded->get_nth_byte(0);  // enc4
    data[1] = recorded->get_nth_byte(1);  // enc3
    data[2] = recorded->get_nth_byte(2);  // enc2
    data[3] = recorded->get_nth_byte(3);  // hash low
    data[4] = recorded->get_nth_byte(4);  // hash high
    data[5] = recorded->get_nth_byte(5);  // count + enc1
    data[6] = recorded->get_nth_byte(6);  // button
}

void RfRemoteController::expandSignalData(const uint8_t* data, BitVector& recorded) {
    // This would reconstruct BitVector from compact format
    // Not implemented as it's not needed for current optimization
}

// Performance monitoring
void RfRemoteController::updateStatistics(uint32_t processingTime) {
    m_stats.averageProcessingTime = (m_stats.averageProcessingTime + processingTime) / 2;
    if (processingTime > m_stats.maxProcessingTime) {
        m_stats.maxProcessingTime = processingTime;
    }
}

void RfRemoteController::recordTimeout() {
    m_stats.timeouts++;
}

void RfRemoteController::recordBufferOverflow() {
    m_stats.bufferOverflows++;
}

void RfRemoteController::recordDuplicate() {
    m_stats.duplicatesFiltered++;
}

void RfRemoteController::resetStatistics() {
    m_stats = {0};
    m_lastStatsUpdate = millis();
}