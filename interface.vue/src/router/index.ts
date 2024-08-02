import { createRouter, createWebHistory } from 'vue-router'

// Framework
import WiFiConnection from '@/framework/wifi/WiFiConnection.vue'
import AccessPoint from '@/framework/ap/AccessPoint.vue'
import NetworkTime from '@/framework/ntp/NetworkTime.vue'
import Mqtt from '@/framework/mqtt/Mqtt.vue'
import Security from '@/framework/security/Security.vue'
import System from '@/framework/system/System.vue'
import SignIn from '@/SignIn.vue'

// Project route
import projectroutes from '@/project/ProjectRoute'
import Project from '@/project/Project.vue'
import { useFeaturesStore } from '@/stores/FeaturesStore'

const ProjectPath = '/' + import.meta.env.VITE_APP_PROJECT_PATH || '/proj';

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: to => {
        const features = useFeaturesStore();

        if(features.data.project)
          return { path: '/' + ProjectPath }
        else 
          return { path: '/wifi' }
      }
    },
    {
      path: ProjectPath,
      name: 'project',
      component: Project,
      children: projectroutes,
      meta: {
        requiredAuth: true
      }
    },
    {
      path: '/wifi/:pathMatch(.*)*',
      name: 'wifi',
      component: WiFiConnection,
      meta: {
        requiredAuth: true
      }
    },
    {
      path: '/ap/:pathMatch(.*)*',
      name: 'ap',
      component: AccessPoint,
      meta: {
        requiredAuth: true
      }
    },
    {
      path: '/ntp/:pathMatch(.*)*',
      name: 'ntp',
      component: NetworkTime,
      meta: {
        requiredAuth: true
      }
    },
    {
      path: '/mqtt/:pathMatch(.*)*',
      name: 'mqtt',
      component: Mqtt,
      meta: {
        requiredAuth: true
      }
    },
    {
      path: '/security/:pathMatch(.*)*',
      name: 'security',
      component: Security,
      meta: {
        requiredAuth: true
      }
    },
    {
      path: '/system/:pathMatch(.*)*',
      name: 'system',
      component: System,
      meta: {
        requiredAuth: true
      }
    },
  ]
})

router.beforeEach((to, from, next) => {
  const features = useFeaturesStore();

  if(to.meta.requiredAuth && features.data.security)
  {
    const token = localStorage.getItem('token');
    if(token)
    {
      // user authenticated, proceed
      next();
    }
    else 
    {
      // not authenticated, redirect to login
      next('/login');
    }
  }
  else 
  {
    // non protected route
    next();
  }
});


export default router
