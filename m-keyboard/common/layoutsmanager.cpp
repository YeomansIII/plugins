/* * This file is part of meego-keyboard *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */



#include "layoutsmanager.h"
#include <algorithm>
#include <QDebug>

namespace
{
    const QString InputMethodLanguages("/meegotouch/inputmethods/languages");
    const QString NumberFormatSettingName("/meegotouch/inputmethods/numberformat");
    const QString InputMethodDefaultLanguage("/meegotouch/inputmethods/languages/default");
    const QString XkbLayoutSettingName("/meegotouch/inputmethods/hwkeyboard/layout");
    const QString XkbVariantSettingName("/meegotouch/inputmethods/hwkeyboard/variant");
    const QString XkbSecondaryLayoutSettingName("/meegotouch/inputmethods/hwkeyboard/secondarylayout");
    const QString XkbModelSettingName("/meegotouch/inputmethods/hwkeyboard/model");
    const QString XkbSecondaryVariantSettingName("/meegotouch/inputmethods/hwkeyboard/secondaryvariant");
    const QString HardwareKeyboardAutoCapsDisabledLayouts("/meegotouch/inputmethods/hwkeyboard/autocapsdisabledlayouts");
    const QString DefaultHardwareKeyboardAutoCapsDisabledLayout("ara"); // Uses xkb layout name. Arabic is "ara".
    const QString SystemDisplayLanguage("/meegotouch/i18n/language");
    const QString DefaultNumberFormat("latin");
    const QString LayoutFileExtension(".xml");
    const QString FallbackLanguage("en_gb");
    const QString FallbackXkbLayout("us");
    const QString NumberKeyboardFileArabic("number_ar.xml");
    const QString NumberKeyboardFileLatin("number.xml");
    const QString PhoneNumberKeyboardFileArabic("phonenumber_ar.xml");
    const QString PhoneNumberKeyboardFileLatin("phonenumber.xml");
    const QString PhoneNumberKeyboardFileRussian("phonenumber_ru.xml");
    const QString SymbolKeyboardFileUs("hwsymbols_us.xml");
    const QString SymbolKeyboardFileEuro("hwsymbols_euro.xml");
    const QString SymbolKeyboardFileArabic("hwsymbols_arabic.xml");
    const QString SymbolKeyboardFileChinese("hwsymbols_chinese.xml");
    const QString FallbackXkbModel("evdev");
}

LayoutsManager *LayoutsManager::Instance = 0;


LayoutsManager::LayoutsManager(const MVirtualKeyboardStyleContainer *newStyleContainer)
    : configLanguages(InputMethodLanguages),
      xkbModelSetting(XkbModelSettingName),
      styleContainer(newStyleContainer),
      hwKeyboard(newStyleContainer),
      numberKeyboard(newStyleContainer),
      phoneNumberKeyboard(newStyleContainer),
      numberFormatSetting(NumberFormatSettingName),
      numberFormat(NumLatin),
      currentHwkbLayoutType(InvalidHardwareKeyboard)
{
    // Read settings for the first time and load keyboard layouts.
    syncLanguages();
    initXkbMap();
    syncHardwareKeyboard();
    syncNumberKeyboards();

    // Synchronize with settings when someone changes them (e.g. via control panel).
    connect(&configLanguages, SIGNAL(valueChanged()), this, SLOT(syncLanguages()));
    connect(&configLanguages, SIGNAL(valueChanged()), this, SIGNAL(selectedLayoutsChanged()));

    connect(&numberFormatSetting, SIGNAL(valueChanged()), this, SLOT(syncNumberKeyboards()));
    connect(&locale, SIGNAL(settingsChanged()), SLOT(syncNumberKeyboards()));
    locale.connectSettings();
}

LayoutsManager::~LayoutsManager()
{
    qDeleteAll(keyboards);
    keyboards.clear();
}

void LayoutsManager::createInstance(const MVirtualKeyboardStyleContainer *styleContainer)
{
    Q_ASSERT(!Instance);
    if (!Instance) {
        Instance = new LayoutsManager(styleContainer);
    }
}

void LayoutsManager::destroyInstance()
{
    Q_ASSERT(Instance);
    delete Instance;
    Instance = 0;
}

int LayoutsManager::languageCount() const
{
    return keyboards.count();
}

QStringList LayoutsManager::languageList() const
{
    // This will return languages in alphabetical ascending order.
    // This means that order in gconf is ignored.
    return keyboards.keys();
}

const KeyboardData *LayoutsManager::keyboardByName(const QString &language) const
{
    QMap<QString, KeyboardData *>::const_iterator kbIter = keyboards.find(language);

    const KeyboardData *keyboard = NULL;
    if (kbIter != keyboards.end())
        keyboard = *kbIter;

    return keyboard;
}

QString LayoutsManager::keyboardTitle(const QString &language) const
{
    const KeyboardData *const keyboard = keyboardByName(language);
    return (keyboard ? keyboard->title() : "");
}

bool LayoutsManager::autoCapsEnabled(const QString &language) const
{
    const KeyboardData *const keyboard = keyboardByName(language);
    return (keyboard ? keyboard->autoCapsEnabled() : false);
}

QString LayoutsManager::keyboardLanguage(const QString &language) const
{
    const KeyboardData *const keyboard = keyboardByName(language);
    return (keyboard ? keyboard->language() : "");
}

const LayoutData *LayoutsManager::layout(const QString &language,
        LayoutData::LayoutType type,
        M::Orientation orientation) const
{
    const LayoutData *lm = 0;

    if (type == LayoutData::Number) {
        lm = numberKeyboard.layout(type, orientation);
    } else if (type == LayoutData::PhoneNumber) {
        lm = phoneNumberKeyboard.layout(type, orientation);
    } else {
        QMap<QString, KeyboardData *>::const_iterator kbIter = keyboards.find(language);

        if (kbIter != keyboards.end() && *kbIter) {
            lm = (*kbIter)->layout(type, orientation);
        }
    }

    if (!lm) {
        static const LayoutData empty;
        lm = &empty;
    }

    return lm;
}

const LayoutData *LayoutsManager::hardwareLayout(LayoutData::LayoutType type,
                                                 M::Orientation orientation) const
{
    return hwKeyboard.layout(type, orientation);
}

QString LayoutsManager::defaultLanguage() const
{
    return MGConfItem(InputMethodDefaultLanguage).value(FallbackLanguage).toString();
}

QString LayoutsManager::systemDisplayLanguage() const
{
    return MGConfItem(SystemDisplayLanguage).value().toString();
}

void LayoutsManager::initXkbMap()
{
    //init current xkb layout and variant.
    setXkbMap(xkbPrimaryLayout(), xkbPrimaryVariant());
}

QString LayoutsManager::xkbModel() const
{
    return xkbModelSetting.value(FallbackXkbModel).toString();
}

QString LayoutsManager::xkbLayout() const
{
    return xkbCurrentLayout;
}

QString LayoutsManager::xkbVariant() const
{
    return xkbCurrentVariant;
}

QString LayoutsManager::xkbPrimaryLayout() const
{
    return MGConfItem(XkbLayoutSettingName).value(FallbackXkbLayout).toString();
}

QString LayoutsManager::xkbPrimaryVariant() const
{
    return MGConfItem(XkbVariantSettingName).value().toString();
}

QString LayoutsManager::xkbSecondaryLayout() const
{
    return MGConfItem(XkbSecondaryLayoutSettingName).value().toString();
}

QString LayoutsManager::xkbSecondaryVariant() const
{
    return MGConfItem(XkbSecondaryVariantSettingName).value().toString();
}

void LayoutsManager::setXkbMap(const QString &layout, const QString &variant)
{
    bool changed = false;
    if (layout != xkbCurrentLayout) {
        changed = true;
        xkbCurrentLayout = layout;
    }

    if (variant != xkbCurrentVariant) {
        changed = true;
        xkbCurrentVariant = variant;
    }

    if (changed) {
        syncHardwareKeyboard();
    }
}

bool LayoutsManager::hardwareKeyboardAutoCapsEnabled() const
{
    // Arabic hwkb layout default disable autocaps.
    QStringList autoCapsDisabledLayouts = MGConfItem(HardwareKeyboardAutoCapsDisabledLayouts)
                                            .value(QStringList(DefaultHardwareKeyboardAutoCapsDisabledLayout))
                                            .toStringList();
    return !(autoCapsDisabledLayouts.contains(xkbLayout()));
}

bool LayoutsManager::loadLanguage(const QString &language)
{
    QMap<QString, KeyboardData *>::iterator kbIterator;

    // sanity tests
    if (language.isEmpty())
        return false;

    KeyboardData *keyboard = new KeyboardData(styleContainer);

    bool loaded = keyboard->loadNokiaKeyboard(language + LayoutFileExtension);

    if (!loaded) {
        loaded = keyboard->loadNokiaKeyboard(QString(language.toLower() + LayoutFileExtension));
    }

    if (!loaded) {
        delete keyboard;
        keyboard = NULL;
        return false;
    }

    // Make sure entry for language exists. Create if it doesn't.
    kbIterator = keyboards.find(language.toLower());
    if (kbIterator == keyboards.end()) {
        kbIterator = keyboards.insert(language.toLower(), 0);
    }

    if (*kbIterator) {
        // What to do?
        qWarning() << "LayoutsManager: Layouts have already been loaded for language "
                   << keyboard->language();
        delete keyboard;
        keyboard = NULL;
        return false;
    }

    *kbIterator = keyboard;

    return true;
}

void LayoutsManager::syncNumberKeyboards()
{
    const QString formatString(numberFormatSetting.value(DefaultNumberFormat).toString().toLower());
    numberFormat = NumLatin;
    if ("arabic" == formatString) {
        numberFormat = NumArabic;
    } else if ("latin" != formatString) {
        qWarning("Invalid value (%s) for number format setting (%s), using Latin.",
                 formatString.toAscii().constData(), NumberFormatSettingName.toAscii().constData());
    }

    bool loaded = numberKeyboard.loadNokiaKeyboard(
                      numberFormat == NumLatin ? NumberKeyboardFileLatin : NumberKeyboardFileArabic);
    // Fall back to Latin if Arabic not available
    if (!loaded && (numberFormat == NumArabic)) {
        numberFormat = NumLatin;
        numberKeyboard.loadNokiaKeyboard(NumberKeyboardFileLatin);
    }

    // Phone number keyboard

    loaded = false;

    if (numberFormat == NumArabic) {
        loaded = phoneNumberKeyboard.loadNokiaKeyboard(PhoneNumberKeyboardFileArabic);
    } else if (locale.categoryLanguage(MLocale::MLcMessages) == "ru") {
        loaded = phoneNumberKeyboard.loadNokiaKeyboard(PhoneNumberKeyboardFileRussian);
    }

    if (!loaded) {
        phoneNumberKeyboard.loadNokiaKeyboard(PhoneNumberKeyboardFileLatin);
    }
    emit numberFormatChanged();
}

void LayoutsManager::syncLanguages()
{
    bool changed = false;
    QStringList newLanguages;
    const QStringList oldLanguages = languageList();

    // Add new ones
    if (!configLanguages.value().isNull()) {
        newLanguages = configLanguages.value().toStringList();

        foreach(QString language, newLanguages) {
            // Existing languages are not reloaded.
            if (!keyboards.contains(language.toLower())) {
                // Add new language
                if (!loadLanguage(language.toLower())) {
                    qWarning() << __PRETTY_FUNCTION__
                               << "New language " << language << " could not be loaded.";
                } else {
                    changed = true;
                }
            }
        }
    }

    // Remove old ones
    foreach(QString old, oldLanguages) {
        if (!newLanguages.contains(old)) {
            keyboards.remove(old);
            changed = true;
        }
    }

    // Try FallbackLanguage if no languages loaded.
    // Don't try to load again if we already tried.
    if (keyboards.isEmpty() && !newLanguages.contains(FallbackLanguage)) {
        if (loadLanguage(FallbackLanguage)) {
            changed = true;
        }
    }

    if (changed) {
        emit languagesChanged();
    }
}

void LayoutsManager::syncHardwareKeyboard()
{
    const QString xkbName = xkbLayout();
    const HardwareKeyboardLayout hwkbLayoutType = xkbLayoutType(xkbName);

    if (hwkbLayoutType == currentHwkbLayoutType) {
        return;
    }

    currentHwkbLayoutType = hwkbLayoutType;

    // What we could do here is to load a generic hw language xml file
    // that would import the correct symbol layout variant but since
    // symbol sections are the only things we currently use, let's just
    // load the hw symbols xml directly.
    const HardwareSymbolVariant symVariant = HwkbLayoutToSymVariant[hwkbLayoutType];
    const QString filename = symbolVariantFileName(symVariant);

    if (hwKeyboard.loadNokiaKeyboard(filename)) {
        emit hardwareLayoutChanged();
    } else {
        qWarning() << "LayoutsManager: loading of hardware layout specific keyboard "
                   << filename << " failed";
    }
}

QString LayoutsManager::symbolVariantFileName(HardwareSymbolVariant symVariant)
{
    QString symFileName;

    switch (symVariant) {
    case HwSymbolVariantUs:
        symFileName = SymbolKeyboardFileUs;
        break;
    case HwSymbolVariantArabic:
        symFileName = SymbolKeyboardFileArabic;
        break;
    case HwSymbolVariantChinese:
        symFileName = SymbolKeyboardFileChinese;
        break;
    case HwSymbolVariantEuro:
    default:
        symFileName = SymbolKeyboardFileEuro;
        break;
    }

    return symFileName;
}

bool LayoutsManager::isCyrillicLanguage(const QString &language)
{
    bool val = false;

    QString shortFormatLanguage = language;
    if (shortFormatLanguage == "ru"    // Russian
            || shortFormatLanguage == "pl" // Polish
            || shortFormatLanguage == "bg" // Bulgaria
            || shortFormatLanguage == "sr" // Serbian
            || shortFormatLanguage == "ky" // Kirghiz
            || shortFormatLanguage == "uk" // Ukrainian
       )
        val = true;
    return val;
}

QMap<QString, QString> LayoutsManager::selectedLayouts() const
{
    QMap<QString, QString> layouts;
    foreach (const QString language, languageList()) {
        layouts.insert(language, keyboardTitle(language));
    }
    return layouts;
}
