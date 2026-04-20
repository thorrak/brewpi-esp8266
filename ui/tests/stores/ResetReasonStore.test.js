import { createTestingPinia } from '@pinia/testing';
import { setActivePinia, createPinia } from 'pinia'
import { useResetReasonStore } from '@/stores/ResetReasonStore.js';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('ResetReasonStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useResetReasonStore();
        expect(store.reason).toBe("");
        expect(store.description).toBe("");
        expect(store.hasResetReason).toBe(false);
        expect(store.resetReasonError).toBe(false);
    });

    it('clears the reset reason correctly', async () => {
        const store = useResetReasonStore();
        store.reason = "Power on";
        store.description = "The device was powered on or reset.";
        store.hasResetReason = true;

        await store.clearResetReason();

        expect(store.reason).toBe("");
        expect(store.description).toBe("");
        expect(store.hasResetReason).toBe(false);
    });

    it('gets reset reason correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, './fixtures/api.resetreason.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useResetReasonStore();

        await store.getResetReason();

        expect(store.reason).toBe(fixtureData.reason);
        expect(store.description).toBe(fixtureData.description);
        expect(store.hasResetReason).toBe(true);
        expect(store.resetReasonError).toBe(false);
    });
});
