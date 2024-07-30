import axios, { type AxiosPromise } from 'axios'
import { jwtDecode } from 'jwt-decode'
import { useRoute, useRouter } from 'vue-router'
import { type Ref, ref } from 'vue'

import { 
    type Features, 
    type SignInRequest, 
    type SignInResponse,
    type Me
} from '@/types'

import { ACCESS_TOKEN, AXIOS } from './endpoints'

export const PROJECT_PATH = import.meta.env.VITE_APP_PROJECT_PATH || "project";
export const SIGN_IN_PATHNAME = 'loginPathname';
export const SIGN_IN_SEARCH = 'loginSearch';

export const getDefaultRoute = (features: Features) => features.project ? `/${PROJECT_PATH}` : "/wifi";

export function verifyAuthorization(): AxiosPromise<void> 
{
    return AXIOS.get('/verifyAuthorization');
}

export function signIn(request: SignInRequest): AxiosPromise<SignInResponse> 
{
    return AXIOS.post('/signIn', request);
}

/**
 * Fallback to sessionStorage if localStorage is absent. WebView may not have local storage enabled.
 */
export function getStorage() 
{
    return localStorage || sessionStorage;
}

export function storeLoginRedirect(location?: { path: string; search: string }) 
{
    if (location) 
    {
      getStorage().setItem(SIGN_IN_PATHNAME, location.path);
      getStorage().setItem(SIGN_IN_SEARCH, location.search);
    }
}
  
export function clearLoginRedirect() 
{
    getStorage().removeItem(SIGN_IN_PATHNAME);
    getStorage().removeItem(SIGN_IN_SEARCH);
}
  
export function fetchLoginRedirect(features: Features): Partial<{ path: string; search?: string }> 
{
    const signInPathname = getStorage().getItem(SIGN_IN_PATHNAME);
    const signInSearch = getStorage().getItem(SIGN_IN_SEARCH);
    clearLoginRedirect();
    return {
      path: signInPathname || getDefaultRoute(features),
      search: (signInPathname && signInSearch) || undefined
    };
}
  
export const clearAccessToken = () => localStorage.removeItem(ACCESS_TOKEN);
export const decodeMeJWT = (accessToken: string): Me => jwtDecode(accessToken) as Me;
  
export function addAccessTokenParameter(url: string) 
{
    const accessToken = getStorage().getItem(ACCESS_TOKEN);
    
    if (!accessToken) {
      return url;
    }

    const parsedUrl = new URL(url);
    parsedUrl.searchParams.set(ACCESS_TOKEN, accessToken);
    return parsedUrl.toString();
}
  

