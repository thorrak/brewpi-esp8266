import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useExtendedSettingsStore = defineStore("ExtendedSettingsStore", () => {
    const hasExtendedSettings = ref(false);
    const extendedSettingsError = ref(false);
    const extendedSettingsUpdateError = ref(false);

    const glycol = ref(false);
    const largeTFT = ref(false);
    const invertTFT = ref(false);
    const resetScreenOnPin = ref(false);

    const SETTINGS_CHOICE = ref(0);
    const MIN_COOL_OFF_TIME = ref(0);
    const MIN_HEAT_OFF_TIME = ref(0);
    const MIN_COOL_ON_TIME = ref(0);
    const MIN_HEAT_ON_TIME = ref(0);
    const MIN_COOL_OFF_TIME_FRIDGE_CONSTANT = ref(0);
    const MIN_SWITCH_TIME = ref(0);
    const COOL_PEAK_DETECT_TIME = ref(0);
    const HEAT_PEAK_DETECT_TIME = ref(0);

    async function getExtendedSettings() {
        try {
            const remote_api = mande("/api/extended/", genCSRFOptions());
            const response = await remote_api.get();
            if (response && response.extendedSettings) {
                hasExtendedSettings.value = true;
                extendedSettingsError.value = false;

                glycol.value = response.extendedSettings.glycol;
                largeTFT.value = response.extendedSettings.largeTFT;
                invertTFT.value = response.extendedSettings.invertTFT;
                resetScreenOnPin.value = response.extendedSettings.resetScreenOnPin;

                SETTINGS_CHOICE.value = response.minTimes.SETTINGS_CHOICE;
                MIN_COOL_OFF_TIME.value = response.minTimes.MIN_COOL_OFF_TIME;
                MIN_HEAT_OFF_TIME.value = response.minTimes.MIN_HEAT_OFF_TIME;
                MIN_COOL_ON_TIME.value = response.minTimes.MIN_COOL_ON_TIME;
                MIN_HEAT_ON_TIME.value = response.minTimes.MIN_HEAT_ON_TIME;
                MIN_COOL_OFF_TIME_FRIDGE_CONSTANT.value = response.minTimes.MIN_COOL_OFF_TIME_FRIDGE_CONSTANT;
                MIN_SWITCH_TIME.value = response.minTimes.MIN_SWITCH_TIME;
                COOL_PEAK_DETECT_TIME.value = response.minTimes.COOL_PEAK_DETECT_TIME;
                HEAT_PEAK_DETECT_TIME.value = response.minTimes.HEAT_PEAK_DETECT_TIME;
            } else {
                await clearExtendedSettings();
                extendedSettingsError.value = true;
            }
        } catch (error) {
            await clearExtendedSettings();
            extendedSettingsError.value = true;
        }
    }

    async function clearExtendedSettings() {
        hasExtendedSettings.value = false;
        glycol.value = false;
        largeTFT.value = false;
        invertTFT.value = false;
        resetScreenOnPin.value = false;

        SETTINGS_CHOICE.value = 0;
        MIN_COOL_OFF_TIME.value = 0;
        MIN_HEAT_OFF_TIME.value = 0;
        MIN_COOL_ON_TIME.value = 0;
        MIN_HEAT_ON_TIME.value = 0;
        MIN_COOL_OFF_TIME_FRIDGE_CONSTANT.value = 0;
        MIN_SWITCH_TIME.value = 0;
        COOL_PEAK_DETECT_TIME.value = 0;
        HEAT_PEAK_DETECT_TIME.value = 0;
    }

    async function setExtendedSettings(glycolInput, largeTFTInput, invertTFTInput, resetScreenOnPinInput, SETTINGS_CHOICEInput, MIN_COOL_OFF_TIMEInput, MIN_HEAT_OFF_TIMEInput, MIN_COOL_ON_TIMEInput, MIN_HEAT_ON_TIMEInput, MIN_COOL_OFF_TIME_FRIDGE_CONSTANTInput, MIN_SWITCH_TIMEInput, COOL_PEAK_DETECT_TIMEInput, HEAT_PEAK_DETECT_TIMEInput) {
        try {
            const remote_api = mande("/api/extended/", genCSRFOptions());
            const response = await remote_api.put({
                glycol: glycolInput,
                largeTFT: largeTFTInput,
                invertTFT: invertTFTInput,
                resetScreenOnPin: resetScreenOnPinInput,
                SETTINGS_CHOICE: SETTINGS_CHOICEInput,
                MIN_COOL_OFF_TIME: MIN_COOL_OFF_TIMEInput,
                MIN_HEAT_OFF_TIME: MIN_HEAT_OFF_TIMEInput,
                MIN_COOL_ON_TIME: MIN_COOL_ON_TIMEInput,
                MIN_HEAT_ON_TIME: MIN_HEAT_ON_TIMEInput,
                MIN_COOL_OFF_TIME_FRIDGE_CONSTANT: MIN_COOL_OFF_TIME_FRIDGE_CONSTANTInput,
                MIN_SWITCH_TIME: MIN_SWITCH_TIMEInput,
                COOL_PEAK_DETECT_TIME: COOL_PEAK_DETECT_TIMEInput,
                HEAT_PEAK_DETECT_TIME: HEAT_PEAK_DETECT_TIMEInput,
            });
            if (response && response.status) {
                glycol.value = glycolInput;
                largeTFT.value = largeTFTInput;
                invertTFT.value = invertTFTInput;
                resetScreenOnPin.value = resetScreenOnPinInput;
                SETTINGS_CHOICE.value = SETTINGS_CHOICEInput;
                MIN_COOL_OFF_TIME.value = MIN_COOL_OFF_TIMEInput;
                MIN_HEAT_OFF_TIME.value = MIN_HEAT_OFF_TIMEInput;
                MIN_COOL_ON_TIME.value = MIN_COOL_ON_TIMEInput;
                MIN_HEAT_ON_TIME.value = MIN_HEAT_ON_TIMEInput;
                MIN_COOL_OFF_TIME_FRIDGE_CONSTANT.value = MIN_COOL_OFF_TIME_FRIDGE_CONSTANTInput;
                MIN_SWITCH_TIME.value = MIN_SWITCH_TIMEInput;
                COOL_PEAK_DETECT_TIME.value = COOL_PEAK_DETECT_TIMEInput;
                HEAT_PEAK_DETECT_TIME.value = HEAT_PEAK_DETECT_TIMEInput;
                extendedSettingsUpdateError.value = false;
            } else {
                await clearExtendedSettings();
                extendedSettingsUpdateError.value = true;
            }
        } catch (error) {
            await clearExtendedSettings();
            extendedSettingsUpdateError.value = true;
        }
    }

    return { hasExtendedSettings, extendedSettingsError, extendedSettingsUpdateError, glycol, largeTFT, invertTFT, resetScreenOnPin, SETTINGS_CHOICE, MIN_COOL_OFF_TIME, MIN_HEAT_OFF_TIME, MIN_COOL_ON_TIME, MIN_HEAT_ON_TIME, MIN_COOL_OFF_TIME_FRIDGE_CONSTANT, MIN_SWITCH_TIME, COOL_PEAK_DETECT_TIME, HEAT_PEAK_DETECT_TIME, getExtendedSettings, clearExtendedSettings, setExtendedSettings };
});
