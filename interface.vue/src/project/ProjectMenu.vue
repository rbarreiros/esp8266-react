<template>
    <NavItem :to="{ name: 'project' }" itemTitle="Demo Project" :mdiIcon="mdiRemote"/>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import NavItem from '@/components/layout/NavItem.vue';
import { mdiRemote } from '@mdi/js';

export default defineComponent({
    name: "ProjectMenu",

    components: {
        NavItem
    },

    setup() {
        return {
            mdiRemote
        }
    },

});
</script>
