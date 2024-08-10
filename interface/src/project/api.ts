import { AxiosPromise } from "axios";
import { AXIOS } from "../api/endpoints";
import { GarageMqttSettings, GarageSettings } from "./types";

export function readBrokerSettings(): AxiosPromise<GarageMqttSettings> {
  return AXIOS.get('/brokerSettings');
}

export function updateBrokerSettings(garageMqttSettings: GarageMqttSettings): AxiosPromise<GarageMqttSettings> {
  return AXIOS.post('/brokerSettings', garageMqttSettings);
}

export function readGarageSettings(): AxiosPromise<GarageSettings> {
  return AXIOS.get('/garageSettings');
}

export function updateGarageSettings(garageSettings: GarageSettings): AxiosPromise<GarageSettings> {
  return AXIOS.post('/garageSettings', garageSettings);
}


