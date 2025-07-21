import { useEffect, FC, useState, useCallback, useRef } from 'react';
import { useSnackbar } from 'notistack';

import { Button, } from '@mui/material';
import PermScanWifiIcon from '@mui/icons-material/PermScanWifi';

import * as WiFiApi from "../../api/wifi";
import { WiFiNetwork, WiFiNetworkList } from '../../types';
import { ButtonRow, FormLoader, SectionContent } from '../../components';
import { extractErrorMessage } from '../../utils';
import { useWs } from '../../utils/useWs';
import { WS_BASE_URL } from '../../api/endpoints';

import WiFiNetworkSelector from './WiFiNetworkSelector';

const WIFI_SCAN_WEBSOCKET_URL = WS_BASE_URL + "wifiScan";

const compareNetworks = (network1: WiFiNetwork, network2: WiFiNetwork) => {
  if (network1.rssi < network2.rssi)
    return 1;
  if (network1.rssi > network2.rssi)
    return -1;
  return 0;
};

interface WiFiScanStatus {
  scanning: boolean;
  scanComplete: boolean;
  networks?: WiFiNetworkList;
  error?: string;
}

const WiFiNetworkScanner: FC = () => {

  const { enqueueSnackbar } = useSnackbar();

  const [networkList, setNetworkList] = useState<WiFiNetworkList>();
  const [errorMessage, setErrorMessage] = useState<string>();
  const [scanning, setScanning] = useState<boolean>(false);

  // Use WebSocket for real-time scan notifications
  const { connected: wsConnected, data: wsData } = useWs<WiFiScanStatus>(WIFI_SCAN_WEBSOCKET_URL);

  const finishedWithError = useCallback((message: string) => {
    enqueueSnackbar(message, { variant: 'error' });
    setNetworkList(undefined);
    setErrorMessage(message);
    setScanning(false);
  }, [enqueueSnackbar]);

  const startScan = useCallback(async () => {
    setScanning(true);
    setErrorMessage(undefined);
    setNetworkList(undefined);
    
    try {
      // Start the scan
      await WiFiApi.scanNetworks();
      
      // Real-time notifications will be handled by WebSocket
      enqueueSnackbar('Network scan started...', { variant: 'info' });
    } catch (error: any) {
      const message = extractErrorMessage(error, 'Failed to start network scan');
      finishedWithError(message);
    }
  }, [enqueueSnackbar, finishedWithError]);

  // Handle real-time scan status updates
  useEffect(() => {
    if (!wsData) return;

    if (wsData.error) {
      finishedWithError(wsData.error);
    } else if (wsData.scanComplete && wsData.networks) {
      const sortedNetworks = wsData.networks.networks.sort(compareNetworks);
      setNetworkList({ networks: sortedNetworks });
      setScanning(false);
      setErrorMessage(undefined);
      enqueueSnackbar(`Found ${sortedNetworks.length} networks`, { variant: 'success' });
    } else if (wsData.scanning) {
      setScanning(true);
    }
  }, [wsData, finishedWithError, enqueueSnackbar]);

  const scanNetworks = () => {
    setScanning(true);
    setErrorMessage(undefined);
    setNetworkList(undefined);
    startScan();
  };

  const renderNetworkScanner = () => {
    if (!networkList) {
      return (
        <SectionContent title="Network Scanner" titleGutter>
          <FormLoader
            message={scanning ? "Scanning networks..." : "Ready to scan"}
            errorMessage={errorMessage}
            onRetry={scanNetworks}
          />
          <ButtonRow>
            <Button
              startIcon={<PermScanWifiIcon />}
              variant="outlined"
              color="secondary"
              onClick={scanNetworks}
              disabled={scanning || !wsConnected}
            >
              {scanning ? 'Scanning...' : 'Scan Networks'}
            </Button>
          </ButtonRow>
        </SectionContent>
      );
    }

    return (
      <WiFiNetworkSelector
        networkList={networkList}
      />
    );
  };

  return renderNetworkScanner();
};

export default WiFiNetworkScanner;
