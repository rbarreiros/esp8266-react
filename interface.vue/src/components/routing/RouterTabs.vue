<template>
    <v-tabs v-model="currentTab" grow>
        <slot name="tabs" :navigate="navigateTo" :current-tab="currentTab"></slot>
    </v-tabs>
    <v-card-text>
      <v-tabs-window v-model="currentTab">
        <slot name="tab-contents" :current-tab="currentTab"></slot>
      </v-tabs-window>
    </v-card-text>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue';
import { useRouter, useRoute } from 'vue-router';

const router = useRouter();
const route = useRoute();
const currentTab = ref(route.path);

// Watch for changes in the route path to update the currentTab
watch(
  () => route.path,
  (newPath) => {
    currentTab.value = newPath;
  }
);

// Handle tab change and navigate to the corresponding route
const navigateTo = (path: string) => {
  router.push(path);
};
</script>
