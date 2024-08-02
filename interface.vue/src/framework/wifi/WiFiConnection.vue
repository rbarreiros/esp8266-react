<script setup lang="ts">
import Layout from '@/components/layout/Layout.vue';
import RouterTabs from '@/components/routing/RouterTabs.vue';
import WiFiStatus from './WiFiStatus.vue';

</script>

<template>
    <Layout>
        <RouterTabs>
            <template #tabs="{ navigate, currentTab }">
                <v-tab :key="1" :value="'status'" @click="navigate('/wifi/status')" :active="currentTab === 'status'">
                    Tab 1
                </v-tab>
                <v-tab :key="2" :value="'scan'" @click="navigate('/wifi/scan')" :active="currentTab === 'scan'">
                    Tab 2
                </v-tab>
                <v-tab :key="3" :value="'settings'" @click="navigate('/wifi/settings')" :active="currentTab === 'settings'">
                    Tab 3
                </v-tab>
            </template>
            <template #tab-contents="{ currentTab }">
                <v-tabs-window-item v-if="currentTab === 'status'" :value="'status'">
                    <div>Content for Tab 1</div>
                </v-tabs-window-item>
                <v-tabs-window-item v-if="currentTab === 'scan'" :value="'scan'">
                    <div>Content for Tab 2</div>
                </v-tabs-window-item>
                <v-tabs-window-item v-if="currentTab === 'settings'" :value="'settings'">
                    <div>Content for Tab 3</div>
                </v-tabs-window-item>
            </template>
        </RouterTabs>
    </Layout>
</template>
