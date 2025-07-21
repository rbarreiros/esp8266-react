import { AxiosPromise } from 'axios';

import { WiFiNetworkList, WiFiSettings, WiFiStatus } from '../types';
import { AXIOS } from './endpoints';

// Compact response adapters for network optimization
interface CompactWiFiStatus {
  sts: number;
  ip: string;
  mac: string;
  rssi: number;
  ssid: string;
  bssid: string;
  ch: number;
  mask: string;
  gw: string;
  dns1: string;
  dns2: string;
  ts?: number;
  v?: number;
}

interface CompactWiFiNetwork {
  r: number;
  s: string;
  b: string;
  c: number;
  e: number;
}

interface CompactWiFiNetworkList {
  networks: CompactWiFiNetwork[];
  ts?: number;
  v?: number;
}

// Convert compact WiFi status to full interface
function adaptWiFiStatus(compact: CompactWiFiStatus): WiFiStatus {
  return {
    status: compact.sts,
    local_ip: compact.ip,
    mac_address: compact.mac,
    rssi: compact.rssi,
    ssid: compact.ssid,
    bssid: compact.bssid,
    channel: compact.ch,
    subnet_mask: compact.mask,
    gateway_ip: compact.gw,
    dns_ip_1: compact.dns1,
    dns_ip_2: compact.dns2,
  };
}

// Convert compact WiFi network list to full interface
function adaptWiFiNetworkList(compact: CompactWiFiNetworkList): WiFiNetworkList {
  return {
    networks: compact.networks.map(network => ({
      rssi: network.r,
      ssid: network.s,
      bssid: network.b,
      channel: network.c,
      encryption_type: network.e,
    })),
  };
}

export function readWiFiStatus(): AxiosPromise<WiFiStatus> {
  return AXIOS.get('/wifiStatus', {
    headers: {
      'Accept-Encoding': 'gzip',
      'Cache-Control': 'no-cache',
    }
  }).then(response => {
    // Check if response has compact format
    if (response.data.sts !== undefined) {
      response.data = adaptWiFiStatus(response.data);
    }
    return response;
  });
}

export function scanNetworks(): AxiosPromise<WiFiNetworkList> {
  return AXIOS.get('/scanNetworks', {
    headers: {
      'Accept-Encoding': 'gzip',
      'Cache-Control': 'no-cache',
    }
  }).then(response => {
    // Check if response has compact format
    if (response.data.networks && response.data.networks.length > 0 && response.data.networks[0].r !== undefined) {
      response.data = adaptWiFiNetworkList(response.data);
    }
    return response;
  });
}

export function listNetworks(): AxiosPromise<WiFiNetworkList> {
  return AXIOS.get('/listNetworks');
}

export function readWiFiSettings(): AxiosPromise<WiFiSettings> {
  return AXIOS.get('/wifiSettings');
}

export function updateWiFiSettings(wifiSettings: WiFiSettings): AxiosPromise<WiFiSettings> {
  return AXIOS.post('/wifiSettings', wifiSettings);
}
