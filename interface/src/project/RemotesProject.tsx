
import { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import { Tab } from '@mui/material';
import { RouterTabs, useRouterTab, useLayoutTitle } from '../components';

import RemotesStateForm from './RemotesStateForm';
import RemotesMqttSettingsForm from './RemotesMqttSettingsForm';

const DemoProject: FC = () => {
  useLayoutTitle("Remotes");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="remotes" label="Remotes" />
        <Tab value="remote_settings" label="MQTT Settings" />
      </RouterTabs>
      <Routes>
        <Route path="remotes" element={<RemotesStateForm />} />
        <Route path="remote_settings" element={<RemotesMqttSettingsForm />} />
        <Route path="/*" element={<Navigate replace to="remotes" />} />
      </Routes>
    </>
  );
};

export default DemoProject;
