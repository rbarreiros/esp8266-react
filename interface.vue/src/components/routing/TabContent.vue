<template>
    <v-tab-item v-if="isVisible" :value="value">
      <slot></slot>
    </v-tab-item>
  </template>
  
  <script setup lang="ts">
  import { defineProps, computed } from 'vue';
  
  const props = defineProps({
    value: { type: String, required: true },
    currentTab: { type: String, required: true },
  });
  
  const isVisible = computed(() => props.currentTab === props.value);
  </script>
  