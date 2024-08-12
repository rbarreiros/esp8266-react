import { FC } from 'react';
import { List } from '@mui/material';
import SettingsRemoteIcon from '@mui/icons-material/SettingsRemote';
import GarageIcon from '@mui/icons-material/GarageRounded'
import { PROJECT_PATH } from '../api/env';
import LayoutMenuItem from '../components/layout/LayoutMenuItem';

const ProjectMenu: FC = () => (
  <List>
    <LayoutMenuItem icon={GarageIcon} label="Garage Door" to={`/${PROJECT_PATH}/garage`} />
    <LayoutMenuItem icon={SettingsRemoteIcon} label="Remotes" to={`/${PROJECT_PATH}/remotes`} />
  </List>
);

export default ProjectMenu;
