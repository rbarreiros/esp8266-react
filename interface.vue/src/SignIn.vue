<template>
    <v-container fluid class="d-flex align-center justify-center fill-height">
        <v-card class="mx-auto pa-10" elevation="8" min-width="460" max-height="430" rounded="lg" height="100%">

            <v-img class="mx-auto" max-width="150" src="/app/icon.png"></v-img>
            <div class="text-h4 text-center pb-4">{{ projectName }}</div>
            <v-form v-model="isValid">
                <v-text-field density="compact" placeholder="Username" variant="outlined" v-model="signInRequest.username"
                    :rules="[v => !!v || 'Username is required']" required>
                </v-text-field>
                <v-text-field :type="visible ? 'text' : 'password'" density="compact" placeholder="Password"
                    variant="outlined" :rules="[v => !!v || 'Password is required']" v-model="signInRequest.password" required>
                    <template v-slot:append-inner>
                        <v-icon @click="visible = !visible">
                            <svg :width="24" :height="24" viewBox="0 0 24 24">
                                <path :d="visible ? mdiEyeOff : mdiEye"></path>
                            </svg>
                        </v-icon>
                    </template>
                </v-text-field>

                <v-btn @click="login" block class="" color="blue" size="large" variant="tonal" :disabled="!isValid">
                    Sign In
                </v-btn>
            </v-form>
        </v-card>
    </v-container>
</template>

<script lang="ts">
import { defineComponent, ref } from 'vue';
import { mdiEyeOff, mdiEye } from '@mdi/js';
import * as AuthenticationApi from './api/authentication';
import { type SignInRequest } from './types';

export default defineComponent({
    name: 'Login',
    setup() {
        const signInRequest = ref<SignInRequest>({
            username: '',
            password: ''
        });
        const isValid = ref<boolean>(true);
        const visible = ref<boolean>(false);
        const projectName = import.meta.env.VITE_APP_PROJECT_NAME || "Default project";

        const login = async () => {
            try {
                const { data: loginResponse } =  await AuthenticationApi.signIn(signInRequest.value);

                // TODO, do login

            } catch (error: any) {
                if(error.response?.status === 401)
                {
                    console.log("Invalid Login");
                }
                else 
                {
                    console.log("Unexpected error");
                }
            }
        };

        return {
            signInRequest,
            isValid,
            login,
            visible,
            projectName,
            mdiEye,
            mdiEyeOff
        };
    },
});
</script>

<style scoped>
.fill-height {
    height: 100vh;
}
</style>