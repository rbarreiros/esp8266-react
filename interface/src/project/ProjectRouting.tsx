import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import GarageProject from './GarageProject';
import RemotesProject from './RemotesProject';

const ProjectRouting: FC = () => {
  return (
    <Routes>
      {
      }
      <Route path="/*" element={<Navigate to="garage/socket" />} />
      {
      }
      <Route path="garage/*" element={<GarageProject />} />
      {
      }
      <Route path="remotes/*" element={<RemotesProject/>} />
    </Routes>
  );
};

export default ProjectRouting;
