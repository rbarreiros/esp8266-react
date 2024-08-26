import Schema from "async-validator";
import { Remote } from "./types";

export const GARAGE_SETTINGS_VALIDATOR = new Schema({
    relay_on_timer: [
        { required: true, message: "Time relay is required, in milliseconds" },
        { type: "number", min: 500, max: 10000, message: "Pin must be between 500 and 10000" }
    ],
})

export const REMOTE_SETTINGS_VALIDATOR = new Schema({
    pairing_timeout: [
        { required: true, message: "Pairing timeout is required, in seconds" },
        { type: "number", min: 30, max: 120, message: "Timeout must be between 30s and 2 minutes (120)" }
    ],
})

export const createRemoteValidator = (remote: Remote[], creating: boolean) => new Schema({
    serial: [
      { required: true, message: "Serial is required" },
      { type: "string", pattern: /^[a-fA-F0-9_\\.]{8}$/, message: "Must be 8 characters: hex string" },
    ],
    button: [
      { required: true, message: "Please provide a button" },
      { type: "number", min: 1, max: 15, message: "Button number must be between 1 and 15" }
    ],
  });
  