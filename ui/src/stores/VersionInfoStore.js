// This version of VersionInfoStore.js is defined as a "setup" store.
import { defineStore } from 'pinia';
import { mande } from 'mande'
import { genCSRFOptions } from './CSRF';
import { ref } from "vue";

export const useVersionInfoStore = defineStore("VersionInfoStore", () => {
    const hasVersionInfo = ref(false);
    const versionInfoError = ref(false);
    const brewpiVersion = ref("");
    const gitRevision = ref("");
    const gitTag = ref("");
    const brewpiShield = ref("");
    const brewpiBoard = ref("");
    const brewpiLogMessagesVersion = ref("");
    const brewpiespVersion = ref("");

    async function getVersionInfo() {
        try {
            const remote_api = mande("/api/version/", genCSRFOptions());
            const response = await remote_api.get();
            if (response && response.v) {
                hasVersionInfo.value = true;
                versionInfoError.value = false;
                brewpiVersion.value = response.v;
                gitRevision.value = response.n;
                gitTag.value = response.c;
                brewpiShield.value = response.s;
                brewpiBoard.value = response.b;
                brewpiLogMessagesVersion.value = response.l;
                brewpiespVersion.value = response.e;
            } else {
                await clearVersionInfo();
                versionInfoError.value = true;
            }
        } catch (error) {
            await clearVersionInfo();
            versionInfoError.value = true;
        }
    }

    async function clearVersionInfo() {
        hasVersionInfo.value = false;
        brewpiVersion.value = "";
        gitRevision.value = "";
        gitTag.value = "";
        brewpiShield.value = "";
        brewpiBoard.value = "";
        brewpiLogMessagesVersion.value = "";
        brewpiespVersion.value = "";
    }

    return { hasVersionInfo, versionInfoError, brewpiVersion, gitRevision, gitTag, brewpiShield, brewpiBoard, brewpiLogMessagesVersion, brewpiespVersion, getVersionInfo, clearVersionInfo };
});
