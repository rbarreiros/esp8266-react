// composables/useSnackbar.ts
import { ref } from 'vue';

export function useSnackbar() {
  const snackbar = ref({
    visible: false,
    message: '',
    color: '',
  });

  const showSnackbar = (message: string, color: string = 'success') => {
    snackbar.value.message = message;
    snackbar.value.color = color;
    snackbar.value.visible = true;
  };

  const closeSnackbar = () => {
    snackbar.value.visible = false;
  };

  return {
    snackbar,
    showSnackbar,
    closeSnackbar,
  };
}
