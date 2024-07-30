<template>
  <v-app>
    <v-navigation-drawer
      app
      v-model="drawer"
      :rail="miniVariant"
    >
      <v-list class="pt-0" dense>
          <ProjectMenu/>
          <v-divider></v-divider>
          <NavItem :to="{ name: 'wifi' }" itemTitle="WiFi Connection" :mdiIcon="mdiWifi"/>
          <NavItem :to="{ name: 'ap' }" itemTitle="Access Point" :mdiIcon="mdiVideoInputAntenna"/>
          <NavItem :to="{ name: 'ntp' }" itemTitle="Network Time" :mdiIcon="mdiClockOutline"/>
          <NavItem :to="{ name: 'mqtt' }" itemTitle="MQTT" :mdiIcon="mdiHubspot"/>
          <NavItem :to="{ name: 'security' }" itemTitle="Security" :mdiIcon="mdiLock"/>
          <NavItem :to="{ name: 'system' }" itemTitle="System" :mdiIcon="mdiCog"/>
        </v-list>
    </v-navigation-drawer>

    <v-app-bar app color="primary" dark>
      <v-app-bar-nav-icon @click.stop="toggleDrawer">
        <v-icon>
          <svg :width="mdiSize" :height="mdiSize" viewBox="0 0 24 24">
            <path :d="mdiMenu"></path>
          </svg>
        </v-icon>
      </v-app-bar-nav-icon>
      <v-toolbar-title>{{ projectName }}</v-toolbar-title>
      <v-spacer></v-spacer>
      <v-btn icon>
        <v-icon>
          <svg :width="mdiSize" :height="mdiSize" viewBox="0 0 24 24">
            <path :d="mdiBell"></path>
          </svg>
        </v-icon>
      </v-btn>
      <v-btn icon>
        <v-icon>
          <svg :width="mdiSize" :height="mdiSize" viewBox="0 0 24 24">
            <path :d="mdiCog"></path>
          </svg>
        </v-icon>
      </v-btn>
    </v-app-bar>

    <v-main>
      <v-container fluid>
        <RouterView />
      </v-container>
    </v-main>
  </v-app>
</template>

<script lang="ts">
import { defineComponent, ref, computed } from 'vue';
import NavItem from '@/components/layout/NavItem.vue';
import ProjectMenu from '@/project/ProjectMenu.vue';

import {
  mdiWifi,
  mdiVideoInputAntenna,
  mdiClockOutline,
  mdiHubspot,
  mdiLock,
  mdiCog,
  mdiMenu,
  mdiBell,
} from '@mdi/js';

export default defineComponent({
  name: 'DrawerLayout',

  components: {
    NavItem,
    ProjectMenu
  },

  setup() {
    const drawer = ref<boolean>(true); // Drawer open by default
    const miniVariant = ref<boolean>(false); // Toggle mini variant based on drawer state
    const mdiSize = '24'; // Size for mdi icons

    const toggleDrawer = () => {
      //drawer.value = !drawer.value;
      miniVariant.value = !miniVariant.value;
    };

    // Access environment variable
    const projectName = import.meta.env.VITE_APP_PROJECT_NAME || 'Vue Project';

    return {
      drawer,
      miniVariant,
      mdiWifi,
      mdiVideoInputAntenna,
      mdiClockOutline,
      mdiHubspot,
      mdiLock,
      mdiSize,
      mdiMenu,
      mdiBell,
      mdiCog,
      toggleDrawer,
      projectName
    };
  }
});
</script>

