<template>
    <v-tab :key="key" :value="value" @click="handleClick" :active="isActive">
      <slot>{{ title }}</slot>
    </v-tab>
  </template>
  
  <script setup lang="ts">
  import { defineProps, defineEmits, computed } from 'vue';
  
  const props = defineProps({
    key: { type: Number, required: true },
    value: { type: String, required: true },
    currentTab: { type: String, required: true },
    title: { type: String, required: true },
  });
  
  const emit = defineEmits(['navigate']);
  
  const handleClick = () => {
    emit('navigate', props.value);
  };
  
  const isActive = computed(() => props.currentTab === props.value);
  </script>
  