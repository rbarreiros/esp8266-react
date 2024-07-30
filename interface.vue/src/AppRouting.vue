<template>
    <RouterView/>
</template>

<script setup lang="ts">
import { useFeaturesStore } from '@/stores/FeaturesStore';

const store = useFeaturesStore();

console.log(store.data);

</script>
