<template>
  <v-app>
    <v-navigation-drawer
      app
      v-model="drawer"
      :rail="miniVariant"
    >
      <LayoutMenu/>
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
        <slot></slot>
      </v-container>
    </v-main>
  </v-app>
</template>

<script lang="ts">
import { defineComponent, ref, computed } from 'vue';
import LayoutMenu from '@/components/layout/LayoutMenu.vue';

import {
  mdiMenu,
  mdiBell,
  mdiCog
} from '@mdi/js';

export default defineComponent({
  name: 'DrawerLayout',

  components: {
    LayoutMenu
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

