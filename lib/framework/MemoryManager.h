#ifndef _MEMORY_MANAGER_H_
#define _MEMORY_MANAGER_H_

#include <Arduino.h>
#include <vector>
#include <map>
#include <ESPAsyncWebServer.h>

#ifdef ESP32
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <esp32-hal-psram.h>
#endif

// Memory management configuration
#define MEMORY_MONITOR_INTERVAL 1000        // 1 second monitoring interval
#define MEMORY_LEAK_DETECTION_ENABLED true  // Enable memory leak detection
#define MEMORY_FRAGMENTATION_THRESHOLD 85   // Alert when fragmentation > 85%
#define MEMORY_LOW_THRESHOLD 8192           // Alert when free memory < 8KB
#define MEMORY_CRITICAL_THRESHOLD 4096     // Critical when free memory < 4KB
#define MEMORY_STACK_THRESHOLD 1024        // Stack usage threshold
#define MEMORY_HISTORY_SIZE 60              // Keep 60 samples of memory history

// Memory pool sizes for optimization
#define SMALL_STRING_POOL_SIZE 32           // 32 small string slots
#define MEDIUM_STRING_POOL_SIZE 16          // 16 medium string slots
#define LARGE_STRING_POOL_SIZE 8            // 8 large string slots
#define SMALL_STRING_SIZE 32                // 32 bytes
#define MEDIUM_STRING_SIZE 128              // 128 bytes
#define LARGE_STRING_SIZE 512               // 512 bytes

// Compression configuration
#define COMPRESSION_MIN_SIZE 150            // Minimum size for compression
#define COMPRESSION_ENABLED true            // Enable compression support

// Memory statistics structure
struct MemoryStats {
    // Current memory state
    uint32_t freeHeap;
    uint32_t totalHeap;
    uint32_t maxAllocHeap;
    uint32_t minFreeHeap;
    uint32_t heapFragmentation;
    
    // PSRAM info (ESP32 only)
    uint32_t freePsram;
    uint32_t totalPsram;
    
    // Stack usage
    uint32_t freeStack;
    uint32_t maxStackUsage;
    
    // Performance metrics
    uint32_t mallocCount;
    uint32_t freeCount;
    uint32_t reallocCount;
    uint32_t failedAllocCount;
    
    // Memory pressure indicators
    uint32_t lowMemoryEvents;
    uint32_t criticalMemoryEvents;
    uint32_t fragmentationEvents;
    
    // Timing
    uint32_t timestamp;
    uint32_t uptimeMs;
};

// Memory allocation tracking (for leak detection)
struct MemoryAllocation {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
    uint32_t timestamp;
    uint32_t stackTrace[4];  // Simple stack trace
};

// String pool entry
struct StringPoolEntry {
    char* buffer;
    size_t size;
    bool inUse;
    uint32_t lastUsed;
    uint32_t useCount;
};

// Memory event callback
typedef std::function<void(const MemoryStats& stats)> MemoryEventCallback;

class MemoryManager {
public:
    // Singleton pattern
    static MemoryManager& getInstance() {
        static MemoryManager instance;
        return instance;
    }
    
    // Initialization and lifecycle
    void begin();
    void loop();
    void end();
    
    // Memory monitoring
    MemoryStats getCurrentStats() const;
    void updateStats();
    void resetStats();
    
    // Memory history
    std::vector<MemoryStats> getMemoryHistory() const { return m_memoryHistory; }
    void clearHistory() { m_memoryHistory.clear(); }
    
    // Event callbacks
    void setLowMemoryCallback(MemoryEventCallback callback) { m_lowMemoryCallback = callback; }
    void setCriticalMemoryCallback(MemoryEventCallback callback) { m_criticalMemoryCallback = callback; }
    void setFragmentationCallback(MemoryEventCallback callback) { m_fragmentationCallback = callback; }
    
    // Memory optimization
    void* allocateOptimized(size_t size, const char* file = nullptr, int line = 0);
    void freeOptimized(void* ptr);
    void* reallocateOptimized(void* ptr, size_t newSize, const char* file = nullptr, int line = 0);
    
    // String pool management
    char* allocateString(size_t size);
    void freeString(char* str);
    void optimizeStringPool();
    
    // Memory analysis
    bool isHeapCorrupted() const;
    uint32_t getHeapFragmentation() const;
    uint32_t getLargestFreeBlock() const;
    uint32_t getStackUsage() const;
    
    // Memory pressure management
    bool isMemoryLow() const;
    bool isMemoryCritical() const;
    void handleMemoryPressure();
    void freeUnusedMemory();
    
    // Leak detection
    void enableLeakDetection(bool enabled) { m_leakDetectionEnabled = enabled; }
    void trackAllocation(void* ptr, size_t size, const char* file, int line, const char* function);
    void trackDeallocation(void* ptr);
    void reportLeaks();
    std::vector<MemoryAllocation> getActiveAllocations() const;
    
    // Debug and diagnostics
    void dumpMemoryInfo() const;
    void dumpStringPool() const;
    void dumpAllocations() const;
    String generateMemoryReport() const;
    
    // Memory protection
    bool validatePointer(void* ptr) const;
    void enableHeapProtection(bool enabled);
    
private:
    MemoryManager();
    ~MemoryManager();
    
    // Disable copy constructor and assignment
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    // Internal state
    MemoryStats m_currentStats;
    std::vector<MemoryStats> m_memoryHistory;
    uint32_t m_lastUpdateTime;
    uint32_t m_startTime;
    
    // String pools
    StringPoolEntry m_smallStringPool[SMALL_STRING_POOL_SIZE];
    StringPoolEntry m_mediumStringPool[MEDIUM_STRING_POOL_SIZE];
    StringPoolEntry m_largeStringPool[LARGE_STRING_POOL_SIZE];
    
    // Leak detection
    bool m_leakDetectionEnabled;
    std::map<void*, MemoryAllocation> m_allocations;
    uint32_t m_nextAllocationId;
    
    // Event callbacks
    MemoryEventCallback m_lowMemoryCallback;
    MemoryEventCallback m_criticalMemoryCallback;
    MemoryEventCallback m_fragmentationCallback;
    
    // Internal methods
    void initializeStringPools();
    void cleanupStringPools();
    StringPoolEntry* findStringPoolEntry(size_t size);
    void compactStringPool(StringPoolEntry* pool, size_t poolSize);
    
    void checkMemoryPressure();
    void triggerMemoryEvent(MemoryEventCallback callback, const MemoryStats& stats);
    
    uint32_t calculateFragmentation() const;
    void updateMemoryHistory();
    
    // Platform-specific implementations
#ifdef ESP32
    void updateESP32Stats();
    uint32_t getESP32StackUsage() const;
#elif defined(ESP8266)
    void updateESP8266Stats();
    uint32_t getESP8266StackUsage() const;
#endif
    
    // Stack trace utilities
    void captureStackTrace(uint32_t* trace, size_t maxDepth);
    void printStackTrace(const uint32_t* trace, size_t depth) const;
};

// Macros for easier memory management
#define MEMORY_MANAGER() MemoryManager::getInstance()
#define MEMORY_STATS() MemoryManager::getInstance().getCurrentStats()

// Memory allocation macros with leak detection
#ifdef MEMORY_LEAK_DETECTION_ENABLED
#define MALLOC_TRACKED(size) MemoryManager::getInstance().allocateOptimized(size, __FILE__, __LINE__)
#define FREE_TRACKED(ptr) MemoryManager::getInstance().freeOptimized(ptr)
#define REALLOC_TRACKED(ptr, size) MemoryManager::getInstance().reallocateOptimized(ptr, size, __FILE__, __LINE__)
#else
#define MALLOC_TRACKED(size) malloc(size)
#define FREE_TRACKED(ptr) free(ptr)
#define REALLOC_TRACKED(ptr, size) realloc(ptr, size)
#endif

// String pool macros
#define STRING_ALLOC(size) MemoryManager::getInstance().allocateString(size)
#define STRING_FREE(str) MemoryManager::getInstance().freeString(str)

// Memory-efficient String class wrapper
class OptimizedString {
private:
    char* m_buffer;
    size_t m_size;
    size_t m_capacity;
    bool m_pooled;
    
public:
    OptimizedString();
    OptimizedString(const char* str);
    OptimizedString(const String& str);
    OptimizedString(const OptimizedString& other);
    ~OptimizedString();
    
    OptimizedString& operator=(const OptimizedString& other);
    OptimizedString& operator=(const String& str);
    OptimizedString& operator=(const char* str);
    
    const char* c_str() const { return m_buffer ? m_buffer : ""; }
    size_t length() const { return m_size; }
    size_t capacity() const { return m_capacity; }
    bool isEmpty() const { return m_size == 0; }
    
    void reserve(size_t size);
    void clear();
    
    OptimizedString& operator+=(const char* str);
    OptimizedString& operator+=(const OptimizedString& other);
    OptimizedString& operator+=(char c);
    
    bool operator==(const OptimizedString& other) const;
    bool operator!=(const OptimizedString& other) const;
    
    String toString() const;
    
private:
    void allocateBuffer(size_t size);
    void freeBuffer();
    void copyFrom(const char* str, size_t len);
};


// HTTP Response Compression Utilities
class CompressionUtils {
public:
    // Check if client accepts GZIP compression
    static bool clientAcceptsGzip(AsyncWebServerRequest* request);
    
    // Send compressed response if beneficial
    static void sendCompressedResponse(AsyncWebServerRequest* request, 
                                     const char* content, 
                                     size_t contentLength,
                                     const char* contentType = "application/json",
                                     const char* cacheControl = "no-cache, must-revalidate");
    
    // Check if compression is beneficial for given content size
    static bool shouldCompress(size_t contentSize);
    
    // Create compressed response headers
    static void addCompressionHeaders(AsyncWebServerResponse* response, bool compressed);
};

#endif // _MEMORY_MANAGER_H_ 