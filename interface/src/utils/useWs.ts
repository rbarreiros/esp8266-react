import { useCallback, useEffect, useRef, useState } from "react";
import { useSnackbar } from 'notistack';

import Sockette from "sockette";

import { ACCESS_TOKEN } from '../api/endpoints';
import { debounce } from ".";

export interface WebSocketMessage<T> {
  type: string;
  origin_id?: string;
  payload?: T;
  delta?: Partial<T>;
  id?: string;
}

export const useWs = <D>(wsUrl: string, wsThrottle: number = 100) => {

  const ws = useRef<Sockette>();
  const clientId = useRef<string>();
  const { enqueueSnackbar } = useSnackbar();

  const [connected, setConnected] = useState<boolean>(false);
  const [data, setData] = useState<D>();
  const [transmit, setTransmit] = useState<boolean>();
  const [clear, setClear] = useState<boolean>();

  const onMessage = useCallback((event: MessageEvent) => {
    const rawData = event.data;
    if (typeof rawData === 'string' || rawData instanceof String) {
      console.log('WebSocket message received:', rawData);
      const message = JSON.parse(rawData as string) as WebSocketMessage<D>;
      console.log('Parsed WebSocket message:', message);
      switch (message.type) {
        case "id":
          clientId.current = message.id;
          console.log('WebSocket client ID set:', message.id);
          break;
        case "payload":
          if (clientId.current) {
            console.log('WebSocket payload received:', message.payload);
            setData((existingData) => (clientId.current === message.origin_id && existingData) || message.payload);
          }
          break;
        case "delta":
          // Handle delta updates - merge with existing data
          if (clientId.current && message.delta) {
            console.log('WebSocket delta received:', message.delta);
            setData((existingData) => {
              if (existingData && clientId.current === message.origin_id) {
                return { ...existingData, ...message.delta };
              }
              return existingData;
            });
          }
          break;
        case "notification":
          // Handle real-time notifications
          if (message.payload) {
            const notification = message.payload as any;
            if (notification.type === 'scan_complete') {
              enqueueSnackbar('Network scan completed', { variant: 'success' });
            } else if (notification.type === 'connection_change') {
              enqueueSnackbar(`Connection status: ${notification.status}`, {
                variant: notification.connected ? 'success' : 'warning'
              });
            }
          }
          break;
      }
    }
  }, [enqueueSnackbar]);

  const doSaveData = useCallback((newData: D, clearData: boolean = false) => {
    if (!ws.current) {
      return;
    }
    if (clearData) {
      setData(undefined);
    }
    ws.current.json(newData);
  }, []);

  const saveData = useRef(debounce(doSaveData, wsThrottle));

  const updateData = (newData: React.SetStateAction<D | undefined>, transmitData: boolean = true, clearData: boolean = false) => {
    setData(newData);
    setTransmit(transmitData);
    setClear(clearData);
  };

  const onOpen = useCallback(() => {
    setConnected(true);
    // Request delta updates for better performance
    if (ws.current) {
      ws.current.json({ type: 'request_delta', enabled: true });
    }
  }, []);

  const onClose = useCallback(() => {
    setConnected(false);
  }, []);

  const onError = useCallback((error: Event) => {
    console.warn('WebSocket error:', error);
    setConnected(false);
  }, []);

  useEffect(() => {
    if (!transmit) {
      return;
    }
    data && saveData.current(data, clear);
    setTransmit(false);
    setClear(false);
  }, [doSaveData, data, transmit, clear]);

  useEffect(() => {
    const accessToken = localStorage.getItem(ACCESS_TOKEN);
    const wsURL = (accessToken ? wsUrl + '?access_token=' + encodeURIComponent(accessToken) : wsUrl);
    console.log('Attempting WebSocket connection to:', wsURL);
    ws.current = new Sockette(wsURL, {
      timeout: 5000,
      maxAttempts: 10,
      onopen: onOpen,
      onmessage: onMessage,
      onreconnect: () => setConnected(true),
      onmaximum: () => setConnected(false),
      onclose: onClose,
      onerror: onError,
    });
    return () => ws.current?.close();
  }, [onOpen, onMessage, onClose, onError, wsUrl]);

  return { connected, updateData, data } as const;
};
