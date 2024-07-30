import { defineStore } from "pinia";
import { type Features } from "@/types";
import * as FeaturesApi from '@/api/features'

export const useFeaturesStore = () => {
    const fStore = defineStore('features', {
        state: () => ({
            data: {
                mqtt: false,
                ntp: false,
                ota: false,
                project: false,
                security: false,
                upload_firmware: false,
            } as Features,
            loaded: false,
        }),
        
        getters: {
            reactiveData(): Features {
                return this.data;
            },
        },

        actions: {
            async fetchFeatures() {
                try {
                    const response = await FeaturesApi.readFeatures();
                    console.log(response);
                    
                    this.data = response.data;
                    this.loaded = true;
                } catch (error: any) {
                    this.loaded = false;
                }
            }
        }
    });

    const featuresStore = fStore();
    
    if(!featuresStore.loaded)
    {
        featuresStore.fetchFeatures();
    }

    return featuresStore;
}


