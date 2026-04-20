import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useControllerActionsStore = defineStore("ControllerActionsStore", () => {
    const actionError = ref(false);
    const actionInProgress = ref(false);
    const lastAction = ref(null);  // 'restart' | 'reset_connection' | 'reset_config'

    async function performAction(action) {
        actionInProgress.value = true;
        lastAction.value = action;
        try {
            const remote_api = mande("/api/action/", genCSRFOptions());
            const response = await remote_api.put({ action: action });
            actionError.value = !(response && response.status);
        } catch (error) {
            actionError.value = true;
        }
        actionInProgress.value = false;
    }

    function clearError() {
        actionError.value = false;
        lastAction.value = null;
    }

    return { actionError, actionInProgress, lastAction, performAction, clearError };
});