import { AxiosPromise } from "axios";
import { AXIOS } from "../api/endpoints";

import { GarageMqttSettings } from "./types";

export function readBrokerSettings(): AxiosPromise<GarageMqttSettings> {
  return AXIOS.get('/brokerSettings');
}

export function updateBrokerSettings(garageMqttSettings: GarageMqttSettings): AxiosPromise<GarageMqttSettings> {
  return AXIOS.post('/brokerSettings', garageMqttSettings);
}




