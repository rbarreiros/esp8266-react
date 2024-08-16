import React from 'react';
//import ReactDOM from 'react-dom';

import { createRoot } from 'react-dom/client';

import { BrowserRouter } from 'react-router-dom';
import App from './App';

/*
ReactDOM.render(
  <React.StrictMode>
    <BrowserRouter>
      <App />
    </BrowserRouter>
  </React.StrictMode>,
  document.getElementById('root')
);
*/
const container = document.getElementById('root');
if(!container) throw new Error("Failed to find container");

createRoot(container).render(
  <React.StrictMode>
    <BrowserRouter>
      <App />
    </BrowserRouter>
  </React.StrictMode>
);