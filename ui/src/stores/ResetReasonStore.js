import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useResetReasonStore = defineStore("ResetReasonStore", () => {
    const reason = ref("");
    const description = ref("");
    const hasResetReason = ref(false);
    const resetReasonError = ref(false);

    async function getResetReason() {
        try {
            const remote_api = mande("/api/resetreason/", genCSRFOptions());
            const response = await remote_api.get();
            if (response && response.reason) {
                reason.value = response.reason;
                description.value = response.description;
                hasResetReason.value = true;
                resetReasonError.value = false;
            } else {
                await clearResetReason();
                resetReasonError.value = true;
            }
        } catch (error) {
            await clearResetReason();
            resetReasonError.value = true;
        }
    }

    async function clearResetReason() {
        reason.value = "";
        description.value = "";
        hasResetReason.value = false;
    }

    return { reason, description, hasResetReason, resetReasonError, getResetReason, clearResetReason };
});
