<template>
  <v-snackbar
    v-model="snackbar.visible"
    :color="snackbar.color"
    timeout="3000"
    top
  >
    {{ snackbar.message }}
    <v-btn variant="text" @click="closeSnackbar">Close</v-btn>
  </v-snackbar>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { useSnackbar } from '@/composables/useSnackbar';
const { snackbar, closeSnackbar } = useSnackbar();
</script>
