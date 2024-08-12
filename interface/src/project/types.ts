export enum GarageStatus {
  STATUS_ERROR    = 0,
  STATUS_CLOSED   = 1,
  STATUS_OPENING  = 2,
  STATUS_CLOSING  = 3,
  STATUS_OPEN     = 4
}

export interface GarageState {
  relay_on: boolean;
  endstop_closed: boolean;
  endstop_open: boolean;
  status: GarageStatus;
}

export interface GarageSettings {
  relay_auto_off: boolean;
  relay_on_timer: number;
}

export interface GarageMqttSettings {
  unique_id: string;
  name: string;
  mqtt_path: string;
}

export interface Remote {
  id: string;
  button: number;
  description: string;
  serial: string;
}

export interface RemoteSettings {
  pairing_timeout: number;
  remotes: Remote[]
}

export interface RemoteState {
  remote_id?: string | undefined;
  remote_button?: number | undefined;
  remote_serial?: string | undefined;
  remote_description?: string | undefined;
  remote_updated_at?: string | undefined;
  pairing?: boolean;
  pairing_error?: string | undefined;
}

export interface RemoteMqttSettings {
  unique_id: string;
  name: string;
  mqtt_path: string;
}
