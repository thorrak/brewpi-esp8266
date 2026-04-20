import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useLCDStore = defineStore("LCDStore", () => {
    const LCDTextLines = ref([
        "Loading data from",
        "controller...",
        "",
        ""
    ]);

    async function getLCD() {
        const remote_api = mande("/api/lcd/", genCSRFOptions());
        const response = await remote_api.get();
        if (response && Array.isArray(response)) {
            LCDTextLines.value = response;
        } else {
            await populateDefaultLCD();
        }
    }

    async function populateDefaultLCD() {
        LCDTextLines.value = [
            "Unable to retrieve",
            "LCD text from device",
            "Check power/internet",
            "connection.",
        ];
    }

    return { LCDTextLines, getLCD, populateDefaultLCD };
});
