import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useHeapInfoStore = defineStore("HeapInfoStore", () => {
    const free = ref(0);
    const max = ref(0);
    const frag = ref(0);
    const hasHeapInfo = ref(false);
    const heapError = ref(false);

    async function getHeapInfo() {
        try {
            const remote_api = mande("/api/heap/", genCSRFOptions());
            const response = await remote_api.get();
            if (response && response.free) {
                free.value = response.free;
                max.value = response.max;
                frag.value = response.frag;
                hasHeapInfo.value = true;
                heapError.value = false;
            } else {
                await clearHeapInfo();
                heapError.value = true;
            }
        } catch (error) {
            await clearHeapInfo();
            heapError.value = true;
        }
    }

    async function clearHeapInfo() {
        free.value = 0;
        max.value = 0;
        frag.value = 0;
        hasHeapInfo.value = false;
    }

    return { free, max, frag, hasHeapInfo, heapError, getHeapInfo, clearHeapInfo };
});
