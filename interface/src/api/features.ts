import { AxiosPromise } from 'axios';

import { Features } from '../types';
import { AXIOS } from './endpoints';

// Compact response adapter for network optimization
interface CompactFeatures {
  proj: boolean | string;
  sec: boolean | string;
  mqtt: boolean | string;
  ntp: boolean | string;
  ota: boolean | string;
  up_fw: boolean | string;
  ts?: number;
  v?: number;
}

// Convert compact features to full interface
function adaptFeatures(compact: CompactFeatures): Features {
  return {
    project: compact.proj === true || compact.proj === "true",
    security: compact.sec === true || compact.sec === "true",
    mqtt: compact.mqtt === true || compact.mqtt === "true",
    ntp: compact.ntp === true || compact.ntp === "true",
    ota: compact.ota === true || compact.ota === "true",
    upload_firmware: compact.up_fw === true || compact.up_fw === "true",
  };
}

export function readFeatures(): AxiosPromise<Features> {
  // Clear any cached features to force fresh request
  localStorage.removeItem('features-etag');
  localStorage.removeItem('cached-features');

  // Add timestamp to force fresh request
  const timestamp = Date.now();

  return AXIOS.get(`/features?t=${timestamp}`, {
    headers: {
      'Accept-Encoding': 'gzip',
      'Cache-Control': 'no-cache, no-store, must-revalidate',
      'Pragma': 'no-cache',
      'Expires': '0',
    }
  }).then((response) => {
    // Store ETag for future requests
    const etag = response.headers['etag'];
    if (etag) {
      localStorage.setItem('features-etag', etag);
    }

    // Cache the response data
    localStorage.setItem('cached-features', JSON.stringify(response.data));

    // Debug logging
    console.log('Features API response:', response.data);

    // Check if response has compact format (either boolean or string values)
    if (response.data.proj !== undefined) {
      console.log('Converting compact features to full format');
      response.data = adaptFeatures(response.data);
      console.log('Converted features:', response.data);
    } else {
      console.log('Features already in full format');
    }
    return response;
  }).catch((error) => {
    // Handle 304 Not Modified by returning cached data
    if (error.response?.status === 304) {
      const cachedFeatures = localStorage.getItem('cached-features');
      if (cachedFeatures) {
        console.log('Using cached features (304 Not Modified)');
        // Create a proper response object for 304
        return Promise.resolve({
          data: JSON.parse(cachedFeatures),
          status: 304,
          statusText: 'Not Modified',
          headers: error.response.headers,
          config: error.config,
        });
      }
    }
    console.error('Features API error:', error);
    throw error;
  });
}
