import { createTestingPinia } from '@pinia/testing';
import { setActivePinia, createPinia } from 'pinia'
import { useLCDStore } from '@/stores/LCDStore.js';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('LCDStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useLCDStore();
        expect(store.LCDTextLines).toEqual([
            "Loading data from",
            "controller...",
            "",
            ""
        ]);
    });

    it('populates default LCD text correctly', async () => {
        const store = useLCDStore();

        await store.populateDefaultLCD();

        expect(store.LCDTextLines).toEqual([
            "Unable to retrieve",
            "LCD text from device",
            "Check power/internet",
            "connection.",
        ]);
    });

    it('gets LCD text correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, './fixtures/api.lcd.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useLCDStore();

        await store.getLCD();

        expect(store.LCDTextLines).toEqual(fixtureData);
    });
});
