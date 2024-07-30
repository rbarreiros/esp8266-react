import { type AxiosPromise } from 'axios';
import { type Features } from '@/types';
import { AXIOS } from './endpoints';

export function readFeatures(): AxiosPromise<Features> 
{
  return AXIOS.get('/features');
}
