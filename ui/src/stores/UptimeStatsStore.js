import { defineStore } from 'pinia';
import { mande } from 'mande'
import { genCSRFOptions } from './CSRF';
import { ref } from 'vue';

export const useUptimeStatsStore = defineStore("UptimeStatsStore", () => {
    const days = ref(0);
    const hours = ref(0);
    const minutes = ref(0);
    const seconds = ref(0);
    const millis = ref(0);
    const hasUptime = ref(false);
    const uptimeError = ref(false);

    async function getUptimeStats() {
        try {
            const remote_api = mande("/api/uptime/", genCSRFOptions());
            const response = await remote_api.get();
            if (response) {
                days.value = response.days;
                hours.value = response.hours;
                minutes.value = response.minutes;
                seconds.value = response.seconds;
                millis.value = response.millis;
                hasUptime.value = true;
                uptimeError.value = false;

            } else {
                await clearUptime();
                uptimeError.value = true;
            }
        } catch (error) {
            await clearUptime();
            uptimeError.value = true;
        }
    }

    async function clearUptime() {
        days.value = 0;
        hours.value = 0;
        minutes.value = 0;
        seconds.value = 0;
        millis.value = 0;
        hasUptime.value = false;
    }

    return { days, hours, minutes, seconds, millis, hasUptime, uptimeError, getUptimeStats, clearUptime };
});
