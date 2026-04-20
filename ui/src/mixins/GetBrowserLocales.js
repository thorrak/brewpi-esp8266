// I'm using this in multiple projects, so breaking it out to make it easier to maintain.


export function getBrowserLocales(options = {}) {
    const defaultOptions = {
        languageCodeOnly: false,
    };

    const opt = {
        ...defaultOptions,
        ...options,
    };

    const browserLocales =
        navigator.languages === undefined
            ? [navigator.language]
            : navigator.languages;

    if (!browserLocales) {
        return undefined;
    }

    return browserLocales.map(locale => {
        const trimmedLocale = locale.trim();
        return opt.languageCodeOnly
            ? trimmedLocale.split(/-|_/)[0]
            : trimmedLocale;
    });
}
