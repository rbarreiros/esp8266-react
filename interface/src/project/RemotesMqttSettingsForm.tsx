
import { FC, useState } from "react";
import { ValidateFieldsError } from "async-validator";
import { Button } from "@mui/material";
import SaveIcon from '@mui/icons-material/Save';
import { ButtonRow, FormLoader, MessageBox, SectionContent, ValidatedTextField } from "../components";
import { validate } from "../validators";
import { useRest, updateValue } from "../utils";

import * as RemotesApi from './api';
import { RemoteMqttSettings } from "./types";
import { REMOTE_MQTT_SETTINGS_VALIDATOR } from "./validators";

const RemotesMqttSettingsForm: FC = () => {
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const {
    loadData, saveData, saving, setData, data, errorMessage
  } = useRest<RemoteMqttSettings>({ read: RemotesApi.readRemoteMqttSettings, update: RemotesApi.updateRemoteMqttSettings });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        await validate(REMOTE_MQTT_SETTINGS_VALIDATOR, data);
        saveData();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    };

    return (
      <>
        <MessageBox
          level="info"
          message="The remotes are controllable via MQTT with the demo project designed to work with Home Assistant's auto discovery feature."
          my={2}
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="unique_id"
          label="Unique ID"
          fullWidth
          variant="outlined"
          value={data.unique_id}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="name"
          label="Name"
          fullWidth
          variant="outlined"
          value={data.name}
          onChange={updateFormValue}
          margin="normal"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="mqtt_path"
          label="MQTT Path"
          fullWidth
          variant="outlined"
          value={data.mqtt_path}
          onChange={updateFormValue}
          margin="normal"
        />
        <ButtonRow mt={1}>
          <Button startIcon={<SaveIcon />} disabled={saving} variant="contained" color="primary" type="submit" onClick={validateAndSubmit}>
            Save
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title='MQTT Settings' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default RemotesMqttSettingsForm;