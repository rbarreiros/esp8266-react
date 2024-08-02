<script setup lang="ts">
import Snackbar from '@/components/layout/Snackbar.vue';
</script>

<template>
    <v-app>
        <Snackbar/>
        <RouterView/>
    </v-app>
</template>

<style>
    #app {
        height: 100vh;
    }
</style>
