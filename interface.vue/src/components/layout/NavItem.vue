<template>
    <v-list-item :to="to">
        <v-list-item-title>{{ itemTitle }}</v-list-item-title>
        <template v-slot:prepend>
            <v-icon>
                <svg :width="mdiSize" :height="mdiSize" viewBox="0 0 24 24">
                    <path :d="mdiIcon"></path>
                </svg>
            </v-icon>
        </template>
    </v-list-item>
</template>

<script lang="ts">
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    name: 'NavItem',
    props: {
        to: {
            type: [String, Object] as PropType<string | Record<string, any>>,
            required: true
        },
        itemTitle: {
            type: String as PropType<string>,
            required: true
        },
        mdiIcon: {
            type: String as PropType<string>,
            required: true
        },
        mdiSize: {
            type: String as PropType<string>,
            default: '24' // Default size for the icon
        }
    }
});
</script>