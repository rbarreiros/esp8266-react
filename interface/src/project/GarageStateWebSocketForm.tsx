
import { FC, useState } from 'react';
import { ValidateFieldsError } from 'async-validator';
import { Avatar, Button, Divider, List, ListItem, ListItemAvatar, ListItemText, Switch, Theme, useTheme } from "@mui/material";
import { WEB_SOCKET_ROOT } from '../api/endpoints';
import { ButtonRow, BlockFormControlLabel, FormLoader, ValidatedTextField, SectionContent } from '../components';
import { numberValue, updateValue, useWs } from '../utils';
import { GarageStatus, GarageState } from './types';
import GarageIcon from '@mui/icons-material/Garage';
import DoNotDisturbOnIcon from '@mui/icons-material/DoNotDisturbOn';

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

const GarageStateWebSocketForm: FC = () => {
  const { connected, updateData, data } = useWs<GarageState>(GARAGE_SETTINGS_WEBSOCKET_URL);
  const [fieldErrors, setFieldErrors] = useState<ValidateFieldsError>();

  const updateFormValue = updateValue(updateData);
  const theme = useTheme();

  const content = () => {
    if (!connected || !data) {
      return (<FormLoader message="Connecting to WebSocket…" />);
    }

    return (
      <>
        
        <SectionContent title='Status' titleGutter>
        <List>
          <ListItem>
            <ListItemAvatar>
              <Avatar sx={{ bgcolor: garageStatusHighlight(data, theme) }}>
                <GarageIcon />
              </Avatar>
            </ListItemAvatar>
            <ListItemText primary="Status" secondary={garageStatus(data)} />
          </ListItem>
          <Divider variant="inset" component="li" />
          <ListItem>
            <ListItemAvatar>
              <Avatar sx={{ bgcolor: endstopClosedStatusHighlight(data, theme) }}>
                <DoNotDisturbOnIcon />
              </Avatar>
            </ListItemAvatar>
            <ListItemText primary="Status" secondary={endstopClosedStatus(data)} />
          </ListItem>
          <Divider variant="inset" component="li" />
          <ListItem>
            <ListItemAvatar>
              <Avatar sx={{ bgcolor: endstopOpenStatusHighlight(data, theme) }}>
                <DoNotDisturbOnIcon />
              </Avatar>
            </ListItemAvatar>
            <ListItemText primary="Status" secondary={endstopOpenStatus(data)} />
          </ListItem>
        </List>
        </SectionContent>
        <SectionContent title='Door' titleGutter>
          <BlockFormControlLabel
            control={
              <Switch
                name="relay_on"
                checked={data.relay_on}
                onChange={updateFormValue}
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
                checked={data.relay_auto_off}
                onChange={updateFormValue}
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
            value={numberValue(data.relay_on_timer)}
            type="number"
            onChange={updateFormValue}
            margin="normal"
        />
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

export default GarageStateWebSocketForm;
