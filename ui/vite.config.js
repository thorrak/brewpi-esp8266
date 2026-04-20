import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import viteCompression from 'vite-plugin-compression';

// eslint-disable-next-line no-undef
const path = require("path");

// let localServer = "http://0.0.0.0:8000/";
const localServer = "http://192.168.5.172/";

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    viteCompression({ verbose: false, deleteOriginFile: true })
  ],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
  build: {
    rollupOptions: {
      output: {
        entryFileNames: `[name].js`,   // works
        chunkFileNames: `[name].js`,   // works
        assetFileNames: `[name].[ext]` // does not work for images
      }
    }
  },
  server: {
    proxy: {
      '/dj-rest-auth': {
        target: localServer,
        changeOrigin: false,
        secure: false,
        onProxyReq: function (request) {
          request.setHeader("origin", localServer);
        }
      },
      '^/api/.*': {
        target: localServer,
        changeOrigin: true,
        // rewrite: (path) => path.replace(/^\/fallback/, '')
      },
    }
  }
})
