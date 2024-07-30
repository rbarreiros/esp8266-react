import Demo from "./Demo.vue";
import Information from "./Information.vue";

const projectroutes = [
    {
      path: '',
      name: 'demo',
      component: Demo,
    },
    {
      path: 'info/',
      name: 'information',
      component: Information,
    },
  ];
  
  export default projectroutes;