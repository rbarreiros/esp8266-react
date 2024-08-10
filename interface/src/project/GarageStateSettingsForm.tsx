
import { FC, useState } from 'react';
import { ValidateFieldsError } from 'async-validator';
import { Avatar, Button, Divider, List, ListItem, ListItemAvatar, ListItemText, Switch, Theme, useTheme } from "@mui/material";
import { WEB_SOCKET_ROOT } from '../api/endpoints';
import { ButtonRow, BlockFormControlLabel, FormLoader, ValidatedTextField, SectionContent } from '../components';
import { numberValue, updateValue, useWs, useRest } from '../utils';
import { GarageStatus, GarageState, GarageSettings } from './types';
import GarageIcon from '@mui/icons-material/Garage';
import SaveIcon from '@mui/icons-material/Save';
import DoNotDisturbOnIcon from '@mui/icons-material/DoNotDisturbOn';
import * as GarageApi from './api'
import { GARAGE_SETTINGS_VALIDATOR } from './validators';
import { validate } from '../validators';

export const GARAGE_SETTINGS_WEBSOCKET_URL = WEB_SOCKET_ROOT + "garageState";

export const garageStatus = ({status}: GarageState) => {
  switch(status)
  {
    case GarageStatus.STATUS_ERROR:
      return "Error";
    case GarageStatus.STATUS_CLOSED:
      return "Closed";
    case GarageStatus.STATUS_OPENING:
      return "Opening";
    case GarageStatus.STATUS_CLOSING:
      return "Closing";
    case GarageStatus.STATUS_OPEN:
      return "Open";
  }
}

export const endstopClosedStatus = ({ endstop_closed }: GarageState) => {
  if(endstop_closed)
    return "Closed endstop active";
  else 
    return "Closed endstop open";
};

export const endstopOpenStatus = ({ endstop_open }: GarageState) => {
  if(endstop_open)
    return "Open endstop active";
  else 
    return "Open endstop open";
};

export const endstopClosedStatusHighlight = ({ endstop_closed }: GarageState, theme: Theme) => {
  if (endstop_closed)
    return theme.palette.success.main;
  else
    return theme.palette.info.main;
};

export const endstopOpenStatusHighlight = ({ endstop_open }: GarageState, theme: Theme) => {
  if (endstop_open)
    return theme.palette.success.main;
  else
    return theme.palette.info.main;
};

const garageStatusHighlight = ({ status }: GarageState, theme: Theme) => {
  switch (status) {
    case GarageStatus.STATUS_CLOSED:
      return theme.palette.info.main;
    case GarageStatus.STATUS_OPEN:
        return theme.palette.success.main;
    case GarageStatus.STATUS_ERROR:
        return theme.palette.error.main;
    case GarageStatus.STATUS_OPENING:
    case GarageStatus.STATUS_CLOSING:  
      return theme.palette.warning.main;
  }
};

const GarageStateSettingsForm: FC = () => {
  const { connected, updateData, data: wsData } = useWs<GarageState>(GARAGE_SETTINGS_WEBSOCKET_URL);
  const {
    loadData, saveData, saving, setData, data: restData, errorMessage
  } = useRest<GarageSettings>({ read: GarageApi.readGarageSettings, update: GarageApi.updateGarageSettings });

  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();

  const updateWsFormValue = updateValue(updateData);
  const updateRestFormValue = updateValue(setData);

  const theme = useTheme();

  const content = () => {
    if (!connected || !wsData) {
      return (<FormLoader message="Connecting to WebSocket…" />);
    }

    if(!restData)
    {
      return (<FormLoader onRetry={loadData} errorMessage={errorMessage} />);
    }

    const validateAndSubmit = async () => {
      try {
        setFieldErrors(undefined);
        await validate(GARAGE_SETTINGS_VALIDATOR, restData);
        saveData();
      } catch (errors: any) {
        setFieldErrors(errors);
      }
    };

    return (
      <>
        
        <SectionContent title='Status' titleGutter>
        <List>
          <ListItem>
            <ListItemAvatar>
              <Avatar sx={{ bgcolor: garageStatusHighlight(wsData, theme) }}>
                <GarageIcon />
              </Avatar>
            </ListItemAvatar>
            <ListItemText primary="Status" secondary={garageStatus(wsData)} />
          </ListItem>
          <Divider variant="inset" component="li" />
          <ListItem>
            <ListItemAvatar>
              <Avatar sx={{ bgcolor: endstopClosedStatusHighlight(wsData, theme) }}>
                <DoNotDisturbOnIcon />
              </Avatar>
            </ListItemAvatar>
            <ListItemText primary="Status" secondary={endstopClosedStatus(wsData)} />
          </ListItem>
          <Divider variant="inset" component="li" />
          <ListItem>
            <ListItemAvatar>
              <Avatar sx={{ bgcolor: endstopOpenStatusHighlight(wsData, theme) }}>
                <DoNotDisturbOnIcon />
              </Avatar>
            </ListItemAvatar>
            <ListItemText primary="Status" secondary={endstopOpenStatus(wsData)} />
          </ListItem>
        </List>
        </SectionContent>
        <SectionContent title='Door' titleGutter>
          <BlockFormControlLabel
            control={
              <Switch
                name="relay_on"
                checked={wsData.relay_on}
                onChange={updateWsFormValue}
                color="primary"
              />
            }
            label="Open door"
          />
        </SectionContent>
        <SectionContent title='Settings' titleGutter>
          <BlockFormControlLabel
            control={
              <Switch
                name="relay_auto_off"
                checked={restData.relay_auto_off}
                onChange={updateRestFormValue}
                color="primary"
              />
            }
            label="Relay Auto Off"
          />
          <ValidatedTextField
            name="relay_on_timer"
            label="Relay On Time"
            fullWidth
            variant="outlined"
            value={numberValue(restData.relay_on_timer)}
            type="number"
            onChange={updateRestFormValue}
            margin="normal"
        />
        <ButtonRow mt={1}>
          <Button startIcon={<SaveIcon />} disabled={saving} variant="contained" color="primary" type="submit" onClick={validateAndSubmit}>
            Save
          </Button>
        </ButtonRow>
        </SectionContent>
      </>
    );
  };

  return (
    <SectionContent title='Garage Controller' titleGutter>
      {content()}
    </SectionContent>
  );
};

export default GarageStateSettingsForm;
