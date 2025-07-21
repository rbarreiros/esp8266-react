import { AxiosPromise } from 'axios';

import { OTASettings, SystemStatus, SystemSettings } from '../types';
import { AXIOS, FileUploadConfig, uploadFile } from './endpoints';

// Compact response adapter for network optimization
interface CompactSystemStatus {
  plt: string;
  max_alloc: number;
  cpu_freq: number;
  mem_free: number;
  sketch_sz: number;
  free_sketch: number;
  sdk_ver: string;
  flash_sz: number;
  flash_spd: number;
  fs_used: number;
  fs_total: number;
  psram_sz?: number;
  psram_free?: number;
  heap_frag?: number;
  mem_pressure?: number;
  mem_events?: number;
  ts?: number;
  v?: number;
}

// Convert compact system status to full interface
function adaptSystemStatus(compact: CompactSystemStatus): SystemStatus {
  const base = {
    esp_platform: compact.plt as any,
    max_alloc_heap: compact.max_alloc,
    cpu_freq_mhz: compact.cpu_freq,
    free_heap: compact.mem_free,
    sketch_size: compact.sketch_sz,
    free_sketch_space: compact.free_sketch,
    sdk_version: compact.sdk_ver,
    flash_chip_size: compact.flash_sz,
    flash_chip_speed: compact.flash_spd,
    fs_used: compact.fs_used,
    fs_total: compact.fs_total,
  };

  // Handle ESP32-specific fields
  if (compact.plt === 'esp32') {
    return {
      ...base,
      esp_platform: compact.plt as any,
      psram_size: compact.psram_sz || 0,
      free_psram: compact.psram_free || 0,
    };
  }

  // Handle ESP8266-specific fields
  return {
    ...base,
    esp_platform: compact.plt as any,
    heap_fragmentation: compact.heap_frag || 0,
  };
}

export function readSystemStatus(timeout?: number): AxiosPromise<SystemStatus> {
  return AXIOS.get('/systemStatus', { 
    timeout,
    headers: {
      'Accept-Encoding': 'gzip',
      'Cache-Control': 'no-cache',
    }
  }).then(response => {
    // Check if response has compact format
    if (response.data.plt) {
      response.data = adaptSystemStatus(response.data);
    }
    return response;
  });
}

export function restart(): AxiosPromise<void> {
  return AXIOS.post('/restart');
}

export function factoryReset(): AxiosPromise<void> {
  return AXIOS.post('/factoryReset');
}

export function readOTASettings(): AxiosPromise<OTASettings> {
  return AXIOS.get('/otaSettings');
}

export function updateOTASettings(otaSettings: OTASettings): AxiosPromise<OTASettings> {
  return AXIOS.post('/otaSettings', otaSettings);
}

export const uploadFirmware = (file: File, config?: FileUploadConfig): AxiosPromise<void> => (
  uploadFile('/uploadFirmware', file, config)
);

export function readSystemSettings(): AxiosPromise<SystemSettings> {
  return AXIOS.get('/systemSettings');
}

export function updateSystemSettings(systemSettings: SystemSettings): AxiosPromise<SystemSettings> {
  return AXIOS.post('/systemSettings', systemSettings);
}
