import { createRouter, createWebHistory } from 'vue-router'

// Framework
import Home from '@/components/layout/Home.vue'
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

const ProjectPath = '/' + import.meta.env.VITE_APP_PROJECT_PATH || '/proj';

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'login',
      component: SignIn
    },
    {
      path: ProjectPath,
      name: 'project',
      component: Project,
      children: projectroutes
    },
    {
      path: '/wifi/:pathMatch(.*)*',
      name: 'wifi',
      component: WiFiConnection
    },
    {
      path: '/ap/:pathMatch(.*)*',
      name: 'ap',
      component: AccessPoint
    },
    {
      path: '/ntp/:pathMatch(.*)*',
      name: 'ntp',
      component: NetworkTime
    },
    {
      path: '/mqtt/:pathMatch(.*)*',
      name: 'mqtt',
      component: Mqtt
    },
    {
      path: '/security/:pathMatch(.*)*',
      name: 'security',
      component: Security
    },
    {
      path: '/system/:pathMatch(.*)*',
      name: 'system',
      component: System
    },



    /*
    {
      path: '/about',
      name: 'about',
      // route level code-splitting
      // this generates a separate chunk (About.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () => import('../views/AboutView.vue')
    }
    */
  ]
})

router.beforeEach((to, from, next) => {
  if(to.meta.requiredAuth)
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
