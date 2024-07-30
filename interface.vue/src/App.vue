<script setup lang="ts">
import Snackbar from '@/components/layout/Snackbar.vue';
import AppRouting from '@/AppRouting.vue';
</script>

<template>
    <v-app>
        <Snackbar/>
        <AppRouting/>
    </v-app>
</template>

<style>
    #app {
        height: 100vh;
    }
</style>
