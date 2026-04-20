import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useControlConstantsStore = defineStore("ControlConstantsStore", () => {
    const hasControlConstants = ref(false);
    const controlConstantsError = ref(false);
    const controlConstantsUpdateError = ref(false);

    const tempFormat = ref("C");
    const tempSetMin = ref(0);
    const tempSetMax = ref(0);
    const pidMax = ref(0);
    const Kp = ref(0);
    const Ki = ref(0);
    const Kd = ref(0);
    const iMaxErr = ref(0);
    const idleRangeH = ref(0);
    const idleRangeL = ref(0);
    const heatTargetH = ref(0);
    const heatTargetL = ref(0);
    const coolTargetH = ref(0);
    const coolTargetL = ref(0);
    const maxHeatTimeForEst = ref(0);
    const maxCoolTimeForEst = ref(0);
    const fridgeFastFilt = ref(0);
    const fridgeSlowFilt = ref(0);
    const fridgeSlopeFilt = ref(0);
    const beerFastFilt = ref(0);
    const beerSlowFilt = ref(0);
    const beerSlopeFilt = ref(0);
    const lah = ref(0);
    const hs = ref(0);

    async function getControlConstants() {
        try {
            const remote_api = mande("/api/cc/", genCSRFOptions());
            const response = await remote_api.get();
            if (response && response.tempFormat !== undefined) {
                hasControlConstants.value = true;
                controlConstantsError.value = false;

                tempFormat.value = response.tempFormat;
                tempSetMin.value = response.tempSetMin;
                tempSetMax.value = response.tempSetMax;
                pidMax.value = response.pidMax;
                Kp.value = response.Kp;
                Ki.value = response.Ki;
                Kd.value = response.Kd;
                iMaxErr.value = response.iMaxErr;
                idleRangeH.value = response.idleRangeH;
                idleRangeL.value = response.idleRangeL;
                heatTargetH.value = response.heatTargetH;
                heatTargetL.value = response.heatTargetL;
                coolTargetH.value = response.coolTargetH;
                coolTargetL.value = response.coolTargetL;
                maxHeatTimeForEst.value = response.maxHeatTimeForEst;
                maxCoolTimeForEst.value = response.maxCoolTimeForEst;
                fridgeFastFilt.value = response.fridgeFastFilt;
                fridgeSlowFilt.value = response.fridgeSlowFilt;
                fridgeSlopeFilt.value = response.fridgeSlopeFilt;
                beerFastFilt.value = response.beerFastFilt;
                beerSlowFilt.value = response.beerSlowFilt;
                beerSlopeFilt.value = response.beerSlopeFilt;
                lah.value = response.lah;
                hs.value = response.hs;
            } else {
                await clearControlConstants();
                controlConstantsError.value = true;
            }
        } catch (error) {
            await clearControlConstants();
            controlConstantsError.value = true;
        }
    }

    async function clearControlConstants() {
        hasControlConstants.value = false;
        tempFormat.value = "C";
        tempSetMin.value = 0;
        tempSetMax.value = 0;
        pidMax.value = 0;
        Kp.value = 0;
        Ki.value = 0;
        Kd.value = 0;
        iMaxErr.value = 0;
        idleRangeH.value = 0;
        idleRangeL.value = 0;
        heatTargetH.value = 0;
        heatTargetL.value = 0;
        coolTargetH.value = 0;
        coolTargetL.value = 0;
        maxHeatTimeForEst.value = 0;
        maxCoolTimeForEst.value = 0;
        fridgeFastFilt.value = 0;
        fridgeSlowFilt.value = 0;
        fridgeSlopeFilt.value = 0;
        beerFastFilt.value = 0;
        beerSlowFilt.value = 0;
        beerSlopeFilt.value = 0;
        lah.value = 0;
        hs.value = 0;
    }

    async function setTempFormat(newTempFormat) {
        try {
            const remote_api = mande("/api/cc/", genCSRFOptions());
            const response = await remote_api.put({
                tempFormat: newTempFormat,
            });
            if (response && response.status) {
                tempFormat.value = newTempFormat;
                controlConstantsUpdateError.value = false;
            } else {
                controlConstantsUpdateError.value = true;
            }
        } catch (error) {
            controlConstantsUpdateError.value = true;
        }
    }

    return {
        hasControlConstants,
        controlConstantsError,
        controlConstantsUpdateError,
        tempFormat,
        tempSetMin,
        tempSetMax,
        pidMax,
        Kp,
        Ki,
        Kd,
        iMaxErr,
        idleRangeH,
        idleRangeL,
        heatTargetH,
        heatTargetL,
        coolTargetH,
        coolTargetL,
        maxHeatTimeForEst,
        maxCoolTimeForEst,
        fridgeFastFilt,
        fridgeSlowFilt,
        fridgeSlopeFilt,
        beerFastFilt,
        beerSlowFilt,
        beerSlopeFilt,
        lah,
        hs,
        getControlConstants,
        clearControlConstants,
        setTempFormat
    };
});
