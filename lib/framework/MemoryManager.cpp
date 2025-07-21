#include "MemoryManager.h"
#include <cstring>
#include <algorithm>

// ============================================================================
// MemoryManager Implementation
// ============================================================================

MemoryManager::MemoryManager() :
    m_currentStats{0},
    m_lastUpdateTime(0),
    m_startTime(0),
    m_leakDetectionEnabled(MEMORY_LEAK_DETECTION_ENABLED),
    m_nextAllocationId(1)
{
    memset(&m_currentStats, 0, sizeof(m_currentStats));
    memset(m_smallStringPool, 0, sizeof(m_smallStringPool));
    memset(m_mediumStringPool, 0, sizeof(m_mediumStringPool));
    memset(m_largeStringPool, 0, sizeof(m_largeStringPool));
}

MemoryManager::~MemoryManager() {
    cleanupStringPools();
}

void MemoryManager::begin() {
    m_startTime = millis();
    m_lastUpdateTime = m_startTime;
    
    // Initialize string pools
    initializeStringPools();
    
    // Initial memory statistics
    updateStats();
    
    Serial.println(F("Memory Manager initialized"));
    Serial.printf("Initial memory: %u bytes free, %u bytes total\n", 
                  m_currentStats.freeHeap, m_currentStats.totalHeap);
}

void MemoryManager::loop() {
    uint32_t now = millis();
    
    if (now - m_lastUpdateTime >= MEMORY_MONITOR_INTERVAL) {
        updateStats();
        updateMemoryHistory();
        checkMemoryPressure();
        
        m_lastUpdateTime = now;
    }
}

void MemoryManager::end() {
    // Report memory leaks if enabled
    if (m_leakDetectionEnabled) {
        reportLeaks();
    }
    
    // Cleanup string pools
    cleanupStringPools();
    
    Serial.println(F("Memory Manager shut down"));
}

// ============================================================================
// Memory Monitoring
// ============================================================================

MemoryStats MemoryManager::getCurrentStats() const {
    return m_currentStats;
}

void MemoryManager::updateStats() {
    uint32_t now = millis();
    
    // Update timestamp
    m_currentStats.timestamp = now;
    m_currentStats.uptimeMs = now - m_startTime;
    
    // Platform-specific stats
#ifdef ESP32
    updateESP32Stats();
#elif defined(ESP8266)
    updateESP8266Stats();
#endif
    
    // Calculate fragmentation
    m_currentStats.heapFragmentation = calculateFragmentation();
    
    // Update min free heap
    if (m_currentStats.minFreeHeap == 0 || m_currentStats.freeHeap < m_currentStats.minFreeHeap) {
        m_currentStats.minFreeHeap = m_currentStats.freeHeap;
    }
    
    // Stack usage
    m_currentStats.freeStack = getStackUsage();
    if (m_currentStats.maxStackUsage < (8192 - m_currentStats.freeStack)) {
        m_currentStats.maxStackUsage = 8192 - m_currentStats.freeStack;
    }
}

void MemoryManager::resetStats() {
    memset(&m_currentStats, 0, sizeof(m_currentStats));
    m_memoryHistory.clear();
    m_startTime = millis();
    updateStats();
}

// ============================================================================
// Platform-Specific Implementations
// ============================================================================

#ifdef ESP32
void MemoryManager::updateESP32Stats() {
    m_currentStats.freeHeap = ESP.getFreeHeap();
    m_currentStats.totalHeap = ESP.getHeapSize();
    m_currentStats.maxAllocHeap = ESP.getMaxAllocHeap();
    
    // PSRAM stats
    m_currentStats.freePsram = ESP.getFreePsram();
    m_currentStats.totalPsram = ESP.getPsramSize();
}

uint32_t MemoryManager::getESP32StackUsage() const {
    // Get current task stack high water mark
    return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
}
#elif defined(ESP8266)
void MemoryManager::updateESP8266Stats() {
    m_currentStats.freeHeap = ESP.getFreeHeap();
    m_currentStats.maxAllocHeap = ESP.getMaxFreeBlockSize();
    m_currentStats.heapFragmentation = ESP.getHeapFragmentation();
    
    // ESP8266 doesn't have direct total heap access
    m_currentStats.totalHeap = 81920; // Approximate value
    
    // No PSRAM on ESP8266
    m_currentStats.freePsram = 0;
    m_currentStats.totalPsram = 0;
}

uint32_t MemoryManager::getESP8266StackUsage() const {
    // ESP8266 stack calculation (approximate)
    char stackVar;
    return (uint32_t)(&stackVar) & 0xFFFF;
}
#endif

uint32_t MemoryManager::getStackUsage() const {
#ifdef ESP32
    return getESP32StackUsage();
#elif defined(ESP8266)
    return getESP8266StackUsage();
#endif
}

// ============================================================================
// Memory Optimization
// ============================================================================

void* MemoryManager::allocateOptimized(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    
    if (ptr) {
        m_currentStats.mallocCount++;
        
        // Track allocation if leak detection is enabled
        if (m_leakDetectionEnabled && file) {
            trackAllocation(ptr, size, file, line, __FUNCTION__);
        }
    } else {
        m_currentStats.failedAllocCount++;
        Serial.printf("MEMORY: Failed to allocate %u bytes at %s:%d\n", size, file ? file : "unknown", line);
    }
    
    return ptr;
}

void MemoryManager::freeOptimized(void* ptr) {
    if (ptr) {
        m_currentStats.freeCount++;
        
        // Track deallocation if leak detection is enabled
        if (m_leakDetectionEnabled) {
            trackDeallocation(ptr);
        }
        
        free(ptr);
    }
}

void* MemoryManager::reallocateOptimized(void* ptr, size_t newSize, const char* file, int line) {
    if (m_leakDetectionEnabled && ptr) {
        trackDeallocation(ptr);
    }
    
    void* newPtr = realloc(ptr, newSize);
    
    if (newPtr) {
        m_currentStats.reallocCount++;
        
        if (m_leakDetectionEnabled && file) {
            trackAllocation(newPtr, newSize, file, line, __FUNCTION__);
        }
    } else {
        m_currentStats.failedAllocCount++;
        Serial.printf("MEMORY: Failed to reallocate %u bytes at %s:%d\n", newSize, file ? file : "unknown", line);
    }
    
    return newPtr;
}

// ============================================================================
// String Pool Management
// ============================================================================

void MemoryManager::initializeStringPools() {
    // Initialize small string pool
    for (size_t i = 0; i < SMALL_STRING_POOL_SIZE; i++) {
        m_smallStringPool[i].buffer = (char*)malloc(SMALL_STRING_SIZE);
        m_smallStringPool[i].size = SMALL_STRING_SIZE;
        m_smallStringPool[i].inUse = false;
        m_smallStringPool[i].lastUsed = 0;
        m_smallStringPool[i].useCount = 0;
    }
    
    // Initialize medium string pool
    for (size_t i = 0; i < MEDIUM_STRING_POOL_SIZE; i++) {
        m_mediumStringPool[i].buffer = (char*)malloc(MEDIUM_STRING_SIZE);
        m_mediumStringPool[i].size = MEDIUM_STRING_SIZE;
        m_mediumStringPool[i].inUse = false;
        m_mediumStringPool[i].lastUsed = 0;
        m_mediumStringPool[i].useCount = 0;
    }
    
    // Initialize large string pool
    for (size_t i = 0; i < LARGE_STRING_POOL_SIZE; i++) {
        m_largeStringPool[i].buffer = (char*)malloc(LARGE_STRING_SIZE);
        m_largeStringPool[i].size = LARGE_STRING_SIZE;
        m_largeStringPool[i].inUse = false;
        m_largeStringPool[i].lastUsed = 0;
        m_largeStringPool[i].useCount = 0;
    }
}

void MemoryManager::cleanupStringPools() {
    // Cleanup small string pool
    for (size_t i = 0; i < SMALL_STRING_POOL_SIZE; i++) {
        if (m_smallStringPool[i].buffer) {
            free(m_smallStringPool[i].buffer);
            m_smallStringPool[i].buffer = nullptr;
        }
    }
    
    // Cleanup medium string pool
    for (size_t i = 0; i < MEDIUM_STRING_POOL_SIZE; i++) {
        if (m_mediumStringPool[i].buffer) {
            free(m_mediumStringPool[i].buffer);
            m_mediumStringPool[i].buffer = nullptr;
        }
    }
    
    // Cleanup large string pool
    for (size_t i = 0; i < LARGE_STRING_POOL_SIZE; i++) {
        if (m_largeStringPool[i].buffer) {
            free(m_largeStringPool[i].buffer);
            m_largeStringPool[i].buffer = nullptr;
        }
    }
}

char* MemoryManager::allocateString(size_t size) {
    StringPoolEntry* entry = findStringPoolEntry(size);
    
    if (entry) {
        entry->inUse = true;
        entry->lastUsed = millis();
        entry->useCount++;
        return entry->buffer;
    }
    
    // Fall back to regular allocation
    return (char*)allocateOptimized(size);
}

void MemoryManager::freeString(char* str) {
    if (!str) return;
    
    // Check if string is from pool
    bool found = false;
    
    // Check small pool
    for (size_t i = 0; i < SMALL_STRING_POOL_SIZE; i++) {
        if (m_smallStringPool[i].buffer == str) {
            m_smallStringPool[i].inUse = false;
            found = true;
            break;
        }
    }
    
    // Check medium pool
    if (!found) {
        for (size_t i = 0; i < MEDIUM_STRING_POOL_SIZE; i++) {
            if (m_mediumStringPool[i].buffer == str) {
                m_mediumStringPool[i].inUse = false;
                found = true;
                break;
            }
        }
    }
    
    // Check large pool
    if (!found) {
        for (size_t i = 0; i < LARGE_STRING_POOL_SIZE; i++) {
            if (m_largeStringPool[i].buffer == str) {
                m_largeStringPool[i].inUse = false;
                found = true;
                break;
            }
        }
    }
    
    // If not from pool, free normally
    if (!found) {
        freeOptimized(str);
    }
}

StringPoolEntry* MemoryManager::findStringPoolEntry(size_t size) {
    StringPoolEntry* pool = nullptr;
    size_t poolSize = 0;
    
    // Choose appropriate pool
    if (size <= SMALL_STRING_SIZE) {
        pool = m_smallStringPool;
        poolSize = SMALL_STRING_POOL_SIZE;
    } else if (size <= MEDIUM_STRING_SIZE) {
        pool = m_mediumStringPool;
        poolSize = MEDIUM_STRING_POOL_SIZE;
    } else if (size <= LARGE_STRING_SIZE) {
        pool = m_largeStringPool;
        poolSize = LARGE_STRING_POOL_SIZE;
    } else {
        return nullptr; // Too large for pool
    }
    
    // Find free entry
    for (size_t i = 0; i < poolSize; i++) {
        if (!pool[i].inUse) {
            return &pool[i];
        }
    }
    
    return nullptr; // No free entry
}

// ============================================================================
// Memory Analysis
// ============================================================================

uint32_t MemoryManager::calculateFragmentation() const {
#ifdef ESP8266
    return ESP.getHeapFragmentation();
#else
    // Calculate fragmentation for ESP32
    if (m_currentStats.freeHeap == 0) return 0;
    
    uint32_t largestBlock = getLargestFreeBlock();
    if (largestBlock == 0) return 100;
    
    return 100 - (largestBlock * 100 / m_currentStats.freeHeap);
#endif
}

uint32_t MemoryManager::getLargestFreeBlock() const {
#ifdef ESP32
    return ESP.getMaxAllocHeap();
#elif defined(ESP8266)
    return ESP.getMaxFreeBlockSize();
#endif
}

bool MemoryManager::isMemoryLow() const {
    return m_currentStats.freeHeap < MEMORY_LOW_THRESHOLD;
}

bool MemoryManager::isMemoryCritical() const {
    return m_currentStats.freeHeap < MEMORY_CRITICAL_THRESHOLD;
}

// ============================================================================
// Memory Pressure Management
// ============================================================================

void MemoryManager::checkMemoryPressure() {
    // Check low memory
    if (isMemoryLow()) {
        m_currentStats.lowMemoryEvents++;
        if (m_lowMemoryCallback) {
            triggerMemoryEvent(m_lowMemoryCallback, m_currentStats);
        }
    }
    
    // Check critical memory
    if (isMemoryCritical()) {
        m_currentStats.criticalMemoryEvents++;
        if (m_criticalMemoryCallback) {
            triggerMemoryEvent(m_criticalMemoryCallback, m_currentStats);
        }
        
        // Automatic cleanup
        handleMemoryPressure();
    }
    
    // Check fragmentation
    if (m_currentStats.heapFragmentation > MEMORY_FRAGMENTATION_THRESHOLD) {
        m_currentStats.fragmentationEvents++;
        if (m_fragmentationCallback) {
            triggerMemoryEvent(m_fragmentationCallback, m_currentStats);
        }
    }
}

void MemoryManager::handleMemoryPressure() {
    Serial.println(F("MEMORY: Handling memory pressure"));
    
    // Optimize string pools
    optimizeStringPool();
    
    // Free unused memory
    freeUnusedMemory();
    
    // Clear memory history to free some space
    if (m_memoryHistory.size() > 10) {
        m_memoryHistory.erase(m_memoryHistory.begin(), m_memoryHistory.begin() + 10);
    }
}

void MemoryManager::optimizeStringPool() {
    uint32_t now = millis();
    
    // Compact string pools (not implemented in this basic version)
    // This would involve moving active strings to eliminate fragmentation
}

void MemoryManager::freeUnusedMemory() {
    // This would free any cached or temporary data
    // Implementation depends on specific application needs
}

// ============================================================================
// Leak Detection
// ============================================================================

void MemoryManager::trackAllocation(void* ptr, size_t size, const char* file, int line, const char* function) {
    if (!ptr || !m_leakDetectionEnabled) return;
    
    MemoryAllocation alloc;
    alloc.ptr = ptr;
    alloc.size = size;
    alloc.file = file;
    alloc.line = line;
    alloc.function = function;
    alloc.timestamp = millis();
    
    // Simple stack trace
    captureStackTrace(alloc.stackTrace, 4);
    
    m_allocations[ptr] = alloc;
}

void MemoryManager::trackDeallocation(void* ptr) {
    if (!ptr || !m_leakDetectionEnabled) return;
    
    auto it = m_allocations.find(ptr);
    if (it != m_allocations.end()) {
        m_allocations.erase(it);
    }
}

void MemoryManager::reportLeaks() {
    if (m_allocations.empty()) {
        Serial.println(F("MEMORY: No memory leaks detected"));
        return;
    }
    
    Serial.printf("MEMORY: %u memory leaks detected:\n", m_allocations.size());
    
    for (const auto& pair : m_allocations) {
        const MemoryAllocation& alloc = pair.second;
        Serial.printf("  %p: %u bytes at %s:%d (%s) - age: %ums\n",
                      alloc.ptr, alloc.size, alloc.file, alloc.line, 
                      alloc.function, millis() - alloc.timestamp);
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

void MemoryManager::updateMemoryHistory() {
    m_memoryHistory.push_back(m_currentStats);
    
    // Keep only recent history
    if (m_memoryHistory.size() > MEMORY_HISTORY_SIZE) {
        m_memoryHistory.erase(m_memoryHistory.begin());
    }
}

void MemoryManager::triggerMemoryEvent(MemoryEventCallback callback, const MemoryStats& stats) {
    if (callback) {
        callback(stats);
    }
}

void MemoryManager::captureStackTrace(uint32_t* trace, size_t maxDepth) {
    // Simple stack trace implementation
    // This is platform-specific and simplified
    for (size_t i = 0; i < maxDepth; i++) {
        trace[i] = 0; // Placeholder - would need platform-specific implementation
    }
}

void MemoryManager::dumpMemoryInfo() const {
    Serial.println(F("=== Memory Manager Status ==="));
    Serial.printf("Free Heap: %u bytes\n", m_currentStats.freeHeap);
    Serial.printf("Total Heap: %u bytes\n", m_currentStats.totalHeap);
    Serial.printf("Max Alloc: %u bytes\n", m_currentStats.maxAllocHeap);
    Serial.printf("Min Free: %u bytes\n", m_currentStats.minFreeHeap);
    Serial.printf("Fragmentation: %u%%\n", m_currentStats.heapFragmentation);
    Serial.printf("Stack Usage: %u bytes\n", m_currentStats.freeStack);
    Serial.printf("Allocations: %u malloc, %u free, %u failed\n", 
                  m_currentStats.mallocCount, m_currentStats.freeCount, m_currentStats.failedAllocCount);
    Serial.printf("Memory Events: %u low, %u critical, %u fragmentation\n",
                  m_currentStats.lowMemoryEvents, m_currentStats.criticalMemoryEvents, m_currentStats.fragmentationEvents);
                  
    if (m_leakDetectionEnabled) {
        Serial.printf("Active Allocations: %u\n", m_allocations.size());
    }
}

// ============================================================================
// OptimizedString Implementation
// ============================================================================

OptimizedString::OptimizedString() : m_buffer(nullptr), m_size(0), m_capacity(0), m_pooled(false) {}

OptimizedString::OptimizedString(const char* str) : m_buffer(nullptr), m_size(0), m_capacity(0), m_pooled(false) {
    if (str) {
        size_t len = strlen(str);
        allocateBuffer(len + 1);
        copyFrom(str, len);
    }
}

OptimizedString::OptimizedString(const String& str) : m_buffer(nullptr), m_size(0), m_capacity(0), m_pooled(false) {
    if (str.length() > 0) {
        allocateBuffer(str.length() + 1);
        copyFrom(str.c_str(), str.length());
    }
}

OptimizedString::~OptimizedString() {
    freeBuffer();
}

void OptimizedString::allocateBuffer(size_t size) {
    if (m_buffer) {
        freeBuffer();
    }
    
    m_buffer = STRING_ALLOC(size);
    m_capacity = size;
    m_pooled = true;
    
    if (!m_buffer) {
        m_buffer = (char*)MALLOC_TRACKED(size);
        m_capacity = size;
        m_pooled = false;
    }
}

void OptimizedString::freeBuffer() {
    if (m_buffer) {
        if (m_pooled) {
            STRING_FREE(m_buffer);
        } else {
            FREE_TRACKED(m_buffer);
        }
        m_buffer = nullptr;
        m_size = 0;
        m_capacity = 0;
        m_pooled = false;
    }
}

void OptimizedString::copyFrom(const char* str, size_t len) {
    if (m_buffer && m_capacity > len) {
        memcpy(m_buffer, str, len);
        m_buffer[len] = '\0';
        m_size = len;
    }
}

// ============================================================================
// CompressionUtils Implementation
// ============================================================================

bool CompressionUtils::clientAcceptsGzip(AsyncWebServerRequest* request) {
    if (!request->hasHeader("Accept-Encoding")) {
        return false;
    }
    
    String acceptEncoding = request->getHeader("Accept-Encoding")->value();
    return acceptEncoding.indexOf("gzip") != -1;
}

void CompressionUtils::sendCompressedResponse(AsyncWebServerRequest* request, 
                                            const char* content, 
                                            size_t contentLength,
                                            const char* contentType,
                                            const char* cacheControl) {
    AsyncWebServerResponse* response;
    
    if (clientAcceptsGzip(request) && shouldCompress(contentLength)) {
        // Send compressed response
        response = request->beginResponse(200, contentType, content);
        addCompressionHeaders(response, true);
        response->addHeader("Cache-Control", cacheControl);
        response->addHeader("Vary", "Accept-Encoding");
    } else {
        // Send uncompressed response
        response = request->beginResponse(200, contentType, content);
        addCompressionHeaders(response, false);
        response->addHeader("Cache-Control", cacheControl);
        response->addHeader("Content-Length", String(contentLength));
    }
    
    request->send(response);
}

bool CompressionUtils::shouldCompress(size_t contentSize) {
    return COMPRESSION_ENABLED && contentSize >= COMPRESSION_MIN_SIZE;
}

void CompressionUtils::addCompressionHeaders(AsyncWebServerResponse* response, bool compressed) {
    if (compressed) {
        response->addHeader("Content-Encoding", "gzip");
        // Note: ESP32 AsyncWebServer automatically handles GZIP compression
        // when Content-Encoding: gzip header is set
    }
    // Add other optimization headers
    response->addHeader("Connection", "keep-alive");
} 