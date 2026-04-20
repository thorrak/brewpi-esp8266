import { defineStore } from 'pinia';
import { mande } from 'mande';
import { genCSRFOptions } from './CSRF';
import { BrewPiSensor } from '@/mixins/BrewPiSensor';
import { ref } from 'vue';

export const useBrewPiSensorStore = defineStore("BrewPiSensorStore", () => {
    const devices = ref([]);
    const loaded = ref(false);
    const devicesError = ref(false);
    const deviceUpdateError = ref(false);

    async function getDevices() {
        try {
            const remote_api = mande("/api/devices/", genCSRFOptions());
            const response = await remote_api.get();
            if (response && Array.isArray(response)) {
                await clearDevices();
                for(let i = 0; i < response.length; i++) {
                    let device = new BrewPiSensor();
                    await device.convertFromBrewPi(response[i]);
                    devices.value.push(device);
                }
                loaded.value = true;
                devicesError.value = false;
            } else {
                await clearDevices();
                devicesError.value = true;
            }
        } catch (error) {
            console.warn(error);
            await clearDevices();
            devicesError.value = true;
        }
    }

    async function sendDeviceDefinition(newDeviceDefinition) {
        try {
            const remote_api = mande("/api/devices/", genCSRFOptions());
            // newDeviceDefinition is assumed to be in the BrewPi native format
            const response = await remote_api.put(newDeviceDefinition);
            deviceUpdateError.value = !(response && response.status);
        } catch (error) {
            deviceUpdateError.value = true;
        }
    }

    async function clearDevices() {
        devices.value = [];
        loaded.value = false;
        deviceUpdateError.value = false;
    }

    function findNextDeviceIndex() {
        let next_index = 0;
        let indices = [];

        if(!loaded.value)
            return -1;  // If we haven't loaded any devices, then we can't find a valid index

        // Fill the array with false (where 20 is MAX_DEVICES as specified in the firmware)
        for(let i = 0; i < 20; i++)
            indices[i] = false;

        // Loop over the loaded devices, and tag all used indices as true
        for(let i = 0; i < devices.value.length; i++) {
            if(devices.value[i].index >= 0) {
                indices[devices.value[i].index] = true;
            }
        }

        // Find the first index that is false, and return it
        for(let i = 0; i < 20; i++)
            if(indices[i] === false)
                return i;

        return -1;  // If we can't find an unused index, return -1
    }

    function findDeviceByFunction(device_function) {
        for(let i = 0; i < devices.value.length; i++) {
            if(devices.value[i].device_function_int === device_function) {
                return devices.value[i];
            }
        }
        return null;
    }

    function hasDeviceWithFunction(device_function) {
        return findDeviceByFunction(device_function) != null;
    }

    function getLowerMacAddress(macAddress) {
        const macAddressParts = macAddress.split(':').map(part => parseInt(part, 16));
        macAddressParts[0] -= 1; // Decrement the first byte by 1
        return macAddressParts.map(part => part.toString(16).padStart(2, '0')).join(':');
    }

    function lowerMacAddressInDevices(targetMac) {
        const lowerMac = getLowerMacAddress(targetMac);
        for (const device of devices.value) {
            if (device.address === lowerMac) {
                return true;
            }
        }
        return false;
    }


    return { devices, loaded, devicesError, deviceUpdateError, getDevices, sendDeviceDefinition, clearDevices, findNextDeviceIndex, findDeviceByFunction, hasDeviceWithFunction, getLowerMacAddress, lowerMacAddressInDevices };
});
