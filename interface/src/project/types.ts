export enum GarageStatus {
  STATUS_ERROR    = 0,
  STATUS_CLOSED   = 1,
  STATUS_OPENING  = 2,
  STATUS_CLOSING  = 3,
  STATUS_OPEN     = 4
}

export interface GarageState {
  relay_on: boolean;
  relay_auto_off: boolean;
  relay_on_timer: number;
  endstop_closed: boolean;
  endstop_open: boolean;
  status: GarageStatus;
}

export interface GarageMqttSettings {
  unique_id: string;
  name: string;
  mqtt_path: string;
}