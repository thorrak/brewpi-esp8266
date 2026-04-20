import { setActivePinia, createPinia } from 'pinia'
import { useUptimeStatsStore } from '@/stores/UptimeStatsStore.js';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('UptimeStatsStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useUptimeStatsStore();
        expect(store.days).toBe(0);
        expect(store.hours).toBe(0);
        expect(store.minutes).toBe(0);
        expect(store.seconds).toBe(0);
        expect(store.millis).toBe(0);
        expect(store.hasUptime).toBe(false);
        expect(store.uptimeError).toBe(false);
    });

    it('clears the uptime stats correctly', async () => {
        const store = useUptimeStatsStore();
        store.days = 1;
        store.hours = 1;
        store.minutes = 1;
        store.seconds = 1;
        store.millis = 1;
        store.hasUptime = true;

        await store.clearUptime();

        expect(store.days).toBe(0);
        expect(store.hours).toBe(0);
        expect(store.minutes).toBe(0);
        expect(store.seconds).toBe(0);
        expect(store.millis).toBe(0);
        expect(store.hasUptime).toBe(false);
    });

    it('gets uptime stats correctly', async () => {
        // Read fixture file and parse JSON
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, 'fixtures/api.uptime.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useUptimeStatsStore();

        await store.getUptimeStats();

        expect(store.days).toBe(fixtureData.days);
        expect(store.hours).toBe(fixtureData.hours);
        expect(store.minutes).toBe(fixtureData.minutes);
        expect(store.seconds).toBe(fixtureData.seconds);
        expect(store.millis).toBe(fixtureData.millis);
        expect(store.hasUptime).toBe(true);
        expect(store.uptimeError).toBe(false);
    });
});
