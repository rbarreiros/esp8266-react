import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';

import GarageProject from './GarageProject';

const ProjectRouting: FC = () => {
  return (
    <Routes>
      {
        // Add the default route for your project below
      }
      <Route path="/*" element={<Navigate to="garage/socket" />} />
      {
        // Add your project page routes below.
      }
      <Route path="garage/*" element={<GarageProject />} />
    </Routes>
  );
};

export default ProjectRouting;
