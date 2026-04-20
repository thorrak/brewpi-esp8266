import { createTestingPinia } from '@pinia/testing';
import { setActivePinia, createPinia } from 'pinia'
import { useUpstreamSettingsStore } from '@/stores/UpstreamSettingsStore.js';
import { mande } from 'mande';
import { jest } from '@jest/globals';
import fs from 'fs';
import path from 'path';

jest.mock('mande');

describe('UpstreamSettingsStore', () => {
    beforeEach(() => {
        setActivePinia(createPinia());
    });

    it('has correct initial state', () => {
        const store = useUpstreamSettingsStore();
        expect(store.hasUpstreamSettings).toBe(false);
        expect(store.upstreamSettingsError).toBe(false);
        expect(store.upstreamHost).toBe("");
        expect(store.upstreamPort).toBe(0);
        expect(store.username).toBe("");
        expect(store.apiKey).toBe("");
        expect(store.upstreamRegistrationError).toBe(7);
        expect(store.deviceID).toBe("");
    });

    it('clears the upstream settings correctly', async () => {
        const store = useUpstreamSettingsStore();
        store.hasUpstreamSettings = true;
        store.upstreamHost = "localhost";
        store.upstreamPort = 8080;
        store.username = "user";
        store.apiKey = "key";
        store.upstreamRegistrationError = 0;
        store.deviceID = "id";

        await store.clearUpstreamSettings();

        expect(store.hasUpstreamSettings).toBe(false);
        expect(store.upstreamHost).toBe("");
        expect(store.upstreamPort).toBe(0);
        expect(store.username).toBe("");
        expect(store.apiKey).toBe("");
        expect(store.upstreamRegistrationError).toBe(7);
        expect(store.deviceID).toBe("");
    });

    it('gets upstream settings correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, './fixtures/api.upstream.json'), 'utf-8'));

        const mockGet = jest.fn().mockResolvedValue(fixtureData);
        mande.mockImplementation(() => {
            return {
                get: mockGet,
            };
        });

        const store = useUpstreamSettingsStore();

        await store.getUpstreamSettings();

        expect(store.hasUpstreamSettings).toBe(true);
        expect(store.upstreamSettingsError).toBe(false);
        expect(store.upstreamHost).toBe(fixtureData.upstreamHost);
        expect(store.upstreamPort).toBe(fixtureData.upstreamPort);
        expect(store.username).toBe(fixtureData.username);
        expect(store.apiKey).toBe(fixtureData.apiKey);
        // TODO - Assert other state properties here as needed
    });

    it('sets upstream settings correctly', async () => {
        const fixtureData = JSON.parse(fs.readFileSync(path.resolve(__dirname, './fixtures/api.upstream.json'), 'utf-8'));

        const mockPut = jest.fn().mockResolvedValue({ message: "Settings updated" });
        mande.mockImplementation(() => {
            return {
                put: mockPut,
            };
        });

        const store = useUpstreamSettingsStore();

        await store.setUpstreamSettings(
            fixtureData.upstreamHost,
            fixtureData.upstreamPort,
            fixtureData.resetDeviceID,
            fixtureData.username,
            fixtureData.apiKey
        );

        expect(store.hasUpstreamSettings).toBe(true);
        expect(store.upstreamSettingsError).toBe(false);
        expect(store.upstreamHost).toBe(fixtureData.upstreamHost);
        expect(store.upstreamPort).toBe(fixtureData.upstreamPort);
        expect(store.username).toBe(fixtureData.username);
        expect(store.apiKey).toBe(fixtureData.apiKey);
        // TODO - Assert other state properties here as needed
    });

});
