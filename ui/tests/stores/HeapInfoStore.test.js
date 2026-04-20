import { createTestingPinia } from '@pinia/testing';
import { setActivePinia, createPinia } from 'pinia'
import { useHeapInfoStore } from '@/stores/HeapInfoStore.js';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('HeapInfoStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useHeapInfoStore();
        expect(store.free).toBe(0);
        expect(store.max).toBe(0);
        expect(store.frag).toBe(0);
        expect(store.hasHeapInfo).toBe(false);
        expect(store.heapError).toBe(false);
    });

    it('clears the heap info correctly', async () => {
        const store = useHeapInfoStore();
        store.free = 10000;
        store.max = 20000;
        store.frag = 500;
        store.hasHeapInfo = true;

        await store.clearHeapInfo();

        expect(store.free).toBe(0);
        expect(store.max).toBe(0);
        expect(store.frag).toBe(0);
        expect(store.hasHeapInfo).toBe(false);
    });

    it('gets heap info correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, './fixtures/api.heap.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useHeapInfoStore();

        await store.getHeapInfo();

        expect(store.free).toBe(fixtureData.free);
        expect(store.max).toBe(fixtureData.max);
        expect(store.frag).toBe(fixtureData.frag);
        expect(store.hasHeapInfo).toBe(true);
        expect(store.heapError).toBe(false);
    });
});
