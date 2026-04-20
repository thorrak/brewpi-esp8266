import { setActivePinia, createPinia } from 'pinia'
import { useVersionInfoStore } from '@/stores/VersionInfoStore.js';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('VersionInfoStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useVersionInfoStore();
        expect(store.hasVersionInfo).toBe(false);
        expect(store.versionInfoError).toBe(false);
    });

    it('clears the version info correctly', async () => {
        const store = useVersionInfoStore();
        store.hasVersionInfo = true;
        store.brewpiVersion = "some version";

        await store.clearVersionInfo();

        expect(store.hasVersionInfo).toBe(false);
        expect(store.brewpiVersion).toBe("");
    });

    it('gets version info correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, 'fixtures/api.version.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useVersionInfoStore();

        await store.getVersionInfo();

        expect(store.brewpiVersion).toBe(fixtureData.v);
        expect(store.gitRevision).toBe(fixtureData.n);
        expect(store.gitTag).toBe(fixtureData.c);
        expect(store.brewpiShield).toBe(fixtureData.s);
        expect(store.brewpiBoard).toBe(fixtureData.b);
        expect(store.brewpiLogMessagesVersion).toBe(fixtureData.l);
        expect(store.brewpiespVersion).toBe(fixtureData.e);
    });
});
