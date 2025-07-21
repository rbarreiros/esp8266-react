#include <SystemStatus.h>
#include <MemoryManager.h>

SystemStatus::SystemStatus(AsyncWebServer* server, SecurityManager* securityManager) 
{
  server->on(SYSTEM_STATUS_SERVICE_PATH,
             HTTP_GET,
             securityManager->wrapRequest(std::bind(&SystemStatus::systemStatus, this, std::placeholders::_1),
                                          AuthenticationPredicates::IS_AUTHENTICATED));
}

void SystemStatus::systemStatus(AsyncWebServerRequest* request) 
{
  // Use standard JSON response
  AsyncJsonResponse* response = new AsyncJsonResponse(false);
  JsonObject root = response->getRoot();
  
  // Platform info (compact field names)
#ifdef ESP32
  root["plt"] = "esp32";
  root["max_heap"] = ESP.getMaxAllocHeap();
  root["psram_sz"] = ESP.getPsramSize();
  root["psram_free"] = ESP.getFreePsram();
#elif defined(ESP8266)
  root["plt"] = "esp8266";
  root["max_heap"] = ESP.getMaxFreeBlockSize();
  root["heap_frag"] = ESP.getHeapFragmentation();
#endif
  
  // Core system stats (compact names)
  root["cpu_mhz"] = ESP.getCpuFreqMHz();
  root["free_heap"] = ESP.getFreeHeap();
  root["sketch_sz"] = ESP.getSketchSize();
  root["sketch_free"] = ESP.getFreeSketchSpace();
  root["sdk_ver"] = ESP.getSdkVersion();
  root["flash_sz"] = ESP.getFlashChipSize();
  root["flash_spd"] = ESP.getFlashChipSpeed();
  
  // Filesystem info (compact)
#ifdef ESP32
  root["fs_total"] = ESPFS.totalBytes();
  root["fs_used"] = ESPFS.usedBytes();
#elif defined(ESP8266)
  FSInfo fs_info;
  ESPFS.info(fs_info);
  root["fs_total"] = fs_info.totalBytes;
  root["fs_used"] = fs_info.usedBytes;
#endif
  
  // Memory stats (most compact representation)
  MemoryStats memStats = MemoryManager::getInstance().getCurrentStats();
  
  // Pack critical memory info into single fields
  root["mem_free"] = memStats.freeHeap;
  root["mem_total"] = memStats.totalHeap;
  root["mem_max_alloc"] = memStats.maxAllocHeap;
  root["mem_min_free"] = memStats.minFreeHeap;
  root["mem_frag"] = memStats.heapFragmentation;
  
  // Stack and performance (compact)
  root["stack_free"] = memStats.freeStack;
  root["stack_max"] = memStats.maxStackUsage;
  root["malloc_cnt"] = memStats.mallocCount;
  root["free_cnt"] = memStats.freeCount;
  root["fail_cnt"] = memStats.failedAllocCount;
  
  // Memory pressure indicators (bit flags for efficiency)
  uint8_t memFlags = 0;
  if (MemoryManager::getInstance().isMemoryLow()) memFlags |= 0x01;
  if (MemoryManager::getInstance().isMemoryCritical()) memFlags |= 0x02;
  if (memStats.lowMemoryEvents > 0) memFlags |= 0x04;
  if (memStats.criticalMemoryEvents > 0) memFlags |= 0x08;
  if (memStats.fragmentationEvents > 0) memFlags |= 0x10;
  
  root["mem_flags"] = memFlags;
  root["mem_events"] = memStats.lowMemoryEvents + memStats.criticalMemoryEvents + memStats.fragmentationEvents;
  root["largest_blk"] = MemoryManager::getInstance().getLargestFreeBlock();
  
  // Uptime (compact)
  root["uptime"] = memStats.uptimeMs;
  
  // Send response
  response->addHeader("Cache-Control", "no-cache, must-revalidate");
  response->setLength();
  request->send(response);
}
