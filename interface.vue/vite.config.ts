import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueJsx from '@vitejs/plugin-vue-jsx'
import vueDevTools from 'vite-plugin-vue-devtools'
// @ts-ignore: no types declaration
import progmemGenerator from './vite-plugin-progmem-generator';
import vuetify from 'vite-plugin-vuetify'

// Define plugin options if needed
const pluginOptions = {
  outputPath: '../lib/framework/WWWData.h',
  bytesPerLine: 20,
  indent: '  ',
  includes: "#include <Arduino.h>\n\n"
};

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueJsx(),
    vueDevTools(),
    progmemGenerator(pluginOptions),
    vuetify({ autoImport: true })
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },
  build: {
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        manualChunks: undefined,
        inlineDynamicImports: true,
        hashCharacters: 'hex',
        assetFileNames: 'js/[name].[hash:6].[ext]',
        chunkFileNames: 'js/[name].[hash:6].js',
        entryFileNames: 'js/[name].[hash:6].js'
      }
    }
  },
  server: {
    proxy: {
      '/rest': {
        target: 'http://192.168.1.32',
        changeOrigin: true,
        //rewrite: (path) => path.replace(/^\/api/, '')
      },
      '/ws': {
        target: 'http://192.168.1.32',
        changeOrigin: true,
        //rewrite: (path) => path.replace(/^\/api/, '')
      }
    }
  }
})
