<template>
    <v-list class="pt-0" dense>
        <ProjectMenu />
        <v-divider></v-divider>
        <NavItem :to="{ name: 'wifi' }" itemTitle="WiFi Connection" :mdiIcon="mdiWifi" />
        <NavItem :to="{ name: 'ap' }" itemTitle="Access Point" :mdiIcon="mdiVideoInputAntenna" />
        <NavItem :to="{ name: 'ntp' }" itemTitle="Network Time" :mdiIcon="mdiClockOutline" v-if="features.data.ntp" />
        <NavItem :to="{ name: 'mqtt' }" itemTitle="MQTT" :mdiIcon="mdiHubspot" v-if="features.data.mqtt" />
        <NavItem :to="{ name: 'security' }" itemTitle="Security" :mdiIcon="mdiLock" v-if="features.data.security" />
        <NavItem :to="{ name: 'system' }" itemTitle="System" :mdiIcon="mdiCog" />
    </v-list>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import { useFeaturesStore } from '@/stores/FeaturesStore';
import NavItem from '@/components/layout/NavItem.vue';
import ProjectMenu from '@/project/ProjectMenu.vue';

import {
    mdiWifi,
    mdiVideoInputAntenna,
    mdiClockOutline,
    mdiHubspot,
    mdiLock,
    mdiCog,
} from '@mdi/js';

export default defineComponent({
    name: "LayoutMenu",

    components: {
        NavItem,
        ProjectMenu
    },

    setup() {
        const features = useFeaturesStore();

        return {
            features,
            mdiWifi,
            mdiVideoInputAntenna,
            mdiClockOutline,
            mdiHubspot,
            mdiLock,
            mdiCog,
        }
    }
});

</script>