import { FC, useState } from 'react';
import { ValidateFieldsError } from 'async-validator';
import * as SystemApi from "../../api/system";
import { SystemSettings } from '../../types';
import { Button, Checkbox } from '@mui/material';
import SaveIcon from '@mui/icons-material/Save';
import { BlockFormControlLabel, ButtonRow, FormLoader, SectionContent, ValidatedTextField } from '../../components';
import { validate, SYSTEM_SETTINGS_VALIDATOR } from '../../validators';
import { numberValue, updateValue, useRest } from '../../utils';

const SystemSettingsForm: FC = () => {
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();
  const {
    loadData, saving, data, setData, saveData, errorMessage
  } = useRest<SystemSettings>({ read: SystemApi.readSystemSettings, update: SystemApi.updateSystemSettings });

  const updateFormValue = updateValue(setData);

  const content = () => {
    if (!data) {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        await validate(SYSTEM_SETTINGS_VALIDATOR, data);
        saveData();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    };

    return (
      <>
        <SectionContent title='Reset Button Settings' titleGutter>
        <BlockFormControlLabel
          control={
            <Checkbox
              name="reset_enabled"
              checked={data.reset_enabled}
              onChange={updateFormValue}
            />
          }
          label="Enable Reset Button?"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="reset_pin"
          label="Reset Pin"
          fullWidth
          variant="outlined"
          value={numberValue(data.reset_pin)}
          type="number"
          onChange={updateFormValue}
          margin="normal"
        />
        <BlockFormControlLabel
          control={
            <Checkbox
                name="reset_pullup"
                checked={data.reset_pullup}
                onChange={updateFormValue}
            />
          }
          label="Reset button pullup?"
        />
        </SectionContent>
        <SectionContent title='WiFi Led Settings' titleGutter>
        <BlockFormControlLabel
          control={
            <Checkbox
              name="wifi_led_enabled"
              checked={data.wifi_led_enabled}
              onChange={updateFormValue}
            />
          }
          label="Enable Wifi Status led?"
        />
        <ValidatedTextField
          fieldErrors={fieldErrors}
          name="wifi_led_pin"
          label="WiFi Led Pin"
          fullWidth
          variant="outlined"
          value={numberValue(data.wifi_led_pin)}
          type="number"
          onChange={updateFormValue}
          margin="normal"
        />
        <BlockFormControlLabel
          control={
            <Checkbox
                name="wifi_led_sink"
                checked={data.wifi_led_sink}
                onChange={updateFormValue}
            />
          }
          label="WiFi Led is sink?"
        />
        </SectionContent>
        <ButtonRow mt={1}>
          <Button startIcon={<SaveIcon />} disabled={saving} variant="contained" color="primary" type="submit" onClick={validateAndSubmit}>
            Save
          </Button>
        </ButtonRow>
      </>
    );
  };

  return (
    <SectionContent title='System Settings' titleGutter>
      { content() }
    </SectionContent>
  );
};

export default SystemSettingsForm;
