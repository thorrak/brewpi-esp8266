import { setActivePinia, createPinia } from 'pinia';
import { useBrewPiSensorStore } from '@/stores/BrewPiSensorStore.js';
import { BrewPiSensor } from '@/mixins/BrewPiSensor';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('BrewPiSensorStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useBrewPiSensorStore();
        expect(store.loaded).toBe(false);
        expect(store.devicesError).toBe(false);
    });

    it('clears the devices correctly', async () => {
        const store = useBrewPiSensorStore();
        store.devices = [{}, {}]; // Mock devices
        store.loaded = true;

        await store.clearDevices();

        expect(store.devices).toEqual([]);
        expect(store.loaded).toBe(false);
    });

    it('gets devices correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, 'fixtures/api.devices.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useBrewPiSensorStore();

        await store.getDevices();

        expect(store.devices.length).toBe(fixtureData.length);
        expect(store.devices[0]).toHaveProperty('chamber');
        expect(store.loaded).toBe(true);
    });

    it('finds device by function correctly', () => {
        const store = useBrewPiSensorStore();
        const mockFunction = 2;

        const mockDevice = new BrewPiSensor();
        mockDevice.device_function_int = mockFunction;

        const mockDevice2 = new BrewPiSensor();
        mockDevice2.device_function_int = mockFunction+1;

        store.devices = [mockDevice, mockDevice2];

        const foundDevice = store.findDeviceByFunction(mockFunction);

        expect(foundDevice).toStrictEqual(mockDevice);
    });

    it('checks for device by function correctly', () => {
        const store = useBrewPiSensorStore();
        const mockFunction = 1;
        const mockDevice = { device_function_int: mockFunction };
        store.devices = [mockDevice];

        const hasDevice = store.hasDeviceWithFunction(mockFunction);

        expect(hasDevice).toBe(true);
    });
});
