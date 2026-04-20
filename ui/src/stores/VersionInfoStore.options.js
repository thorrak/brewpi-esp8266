// This version of VersionInfoStore.js is defined as an "options" store.
import { defineStore } from 'pinia';
import { mande } from 'mande'
import { genCSRFOptions } from './CSRF';

export const useVersionInfoStore = defineStore("VersionInfoStore", {
    state: () => {
        return {
            hasVersionInfo: false,
            versionInfoError: false,
            brewpiVersion: "",
            gitRevision: "",
            gitTag: "",
            brewpiShield: "",
            brewpiBoard: "",
            brewpiLogMessagesVersion: "",
            brewpiespVersion: "",
        };
    },
    actions: {
        async getVersionInfo() {
            try {
                const remote_api = mande("/api/version/", genCSRFOptions());
                const response = await remote_api.get();
                if (response && response.v) {
                    this.hasVersionInfo = true;
                    this.versionInfoError = false;
                    this.brewpiVersion = response.v;
                    this.gitRevision = response.n;
                    this.gitTag = response.c;
                    this.brewpiShield = response.s;
                    this.brewpiBoard = response.b;
                    this.brewpiLogMessagesVersion = response.l;
                    this.brewpiespVersion = response.e;
                } else {
                    await this.clearVersionInfo();
                    this.versionInfoError = true;
                }
            } catch (error) {
                await this.clearVersionInfo();
                this.versionInfoError = true;
            }
        },
        async clearVersionInfo() {
            this.hasVersionInfo = false;
            this.brewpiVersion = "";
            this.gitRevision = "";
            this.gitTag = "";
            this.brewpiShield = "";
            this.brewpiBoard = "";
            this.brewpiLogMessagesVersion = "";
            this.brewpiespVersion = "";
        }
    },
});