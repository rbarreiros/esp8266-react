import { AxiosPromise } from "axios";
import { AXIOS } from "../api/endpoints";
import { GarageMqttSettings, GarageSettings, RemoteSettings, RemoteMqttSettings, Remote } from "./types";

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

export function readRemoteSettings(): AxiosPromise<RemoteSettings> {
  return AXIOS.get('/remoteSettings');
}

export function updateRemoteSettings(remoteSettings: RemoteSettings): AxiosPromise<RemoteSettings> {
  return AXIOS.post('/remoteSettings', remoteSettings);
}

export function readRemoteMqttSettings(): AxiosPromise<RemoteMqttSettings> {
  return AXIOS.get('/remoteMqttSettings');
}

export function updateRemoteMqttSettings(remoteMqttSettings: RemoteMqttSettings): AxiosPromise<RemoteMqttSettings> {
  return AXIOS.post('/remoteMqttSettings', remoteMqttSettings);
}

export function createRemote(remote: Remote | void): AxiosPromise<void> {
  return AXIOS.post('/remote/create', remote);
}

export function readRemote(): AxiosPromise<Remote> {
  return AXIOS.get('/remote/read')
}

export function removeRemote(remote: void | Remote): AxiosPromise<void> {
  return AXIOS.post('/remote/delete', remote);
}

export function updateRemote(remote: Remote | void): AxiosPromise<void> {
  return AXIOS.post('/remote/update', remote);
}
