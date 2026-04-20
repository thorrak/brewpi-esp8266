import { createApp } from 'vue'
import { createPinia } from 'pinia'
import { LoadingPlugin } from 'vue-loading-overlay';
import { getBrowserLocales } from './mixins/GetBrowserLocales'

import router from './router'
import App from './App.vue'

import './style.css'
import 'vue-loading-overlay/dist/css/index.css';


// import translations
import de from "./locales/de.json";
import es from "./locales/es.json";
import en from "./locales/en.json";
import nl from "./locales/nl.json";
import pt from "./locales/pt.json";

// configure i18n
import { createI18n } from "vue-i18n";
// console.log("getBrowserLocales: ", getBrowserLocales({ languageCodeOnly: true })[0]);
export const i18n = createI18n({
    // legacy: false,
    locale: getBrowserLocales({ languageCodeOnly: true })[0] || "en",
    fallbackLocale: "en",
    messages: { de, es, en, nl, pt },
});


const pinia = createPinia();
const app = createApp(App);
app.use(pinia);
app.use(i18n);
app.use(LoadingPlugin);
app.use(router);
app.mount('#app');

