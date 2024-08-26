
import { FC } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import { Tab } from '@mui/material';
import { RouterTabs, useRouterTab, useLayoutTitle } from '../components';

import GarageStateSettingsForm from './GarageStateSettingsForm';

const DemoProject: FC = () => {
  useLayoutTitle("Garage Project");
  const { routerTab } = useRouterTab();

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="socket" label="Garage door state" />
      </RouterTabs>
      <Routes>
        <Route path="socket" element={<GarageStateSettingsForm />} />
        <Route path="/*" element={<Navigate replace to="socket" />} />
      </Routes>
    </>
  );
};

export default DemoProject;
