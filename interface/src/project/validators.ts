import Schema from "async-validator";

export const GARAGE_MQTT_SETTINGS_VALIDATOR = new Schema({
    unique_id: {
        required: true, message: "Please provide an id"
    },
    name: {
        required: true, message: "Please provide a name"
    },
    mqtt_path: {
        required: true, message: "Please provide an MQTT path"
    }
});

export const GARAGE_SETTINGS_VALIDATOR = new Schema({
    relay_on_timer: [
        { required: true, message: "Time relay is required, in milliseconds" },
        { type: "number", min: 500, max: 10000, message: "Pin must be between 500 and 10000" }
    ],
})