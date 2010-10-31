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



#include "ut_mimkey.h"

#include "mimabstractkey.h"
#include "mimabstractkeyareastyle.h"
#include "mimkey.h"
#include "mimkeyarea.h"
#include "mimkeymodel.h"
#include "utils.h"

#include <MApplication>
#include <MTheme>

#include <QSignalSpy>
#include <QDebug>

Q_DECLARE_METATYPE(QList<Ut_KeyButton::DirectionPair>)

void Ut_KeyButton::initTestCase()
{
    qRegisterMetaType< QList<DirectionPair> >("QList<DirectionPair>");

    static int argc = 2;
    static char *app_name[] = { (char*) "ut_keybutton",
                                (char *) "-local-theme" };

    disableQtPlugins();
    app = new MApplication(argc, app_name);

    style = new MImAbstractKeyAreaStyleContainer;
    style->initialize("", "", 0);

    parent = new QGraphicsWidget;
    dataKey = createDataKey();
}

void Ut_KeyButton::cleanupTestCase()
{
    delete style;
    delete dataKey;
    delete app;
    app = 0;
    delete parent;
}

void Ut_KeyButton::init()
{
    subject = new MImKey(*dataKey, *style, *parent);
}

void Ut_KeyButton::cleanup()
{
    delete subject;
    subject = 0;
}

void Ut_KeyButton::testSetModifier_data()
{
    QTest::addColumn<bool>("shift");
    QTest::addColumn<QChar>("accent");
    QTest::addColumn<QString>("expectedLabel");

    QChar grave(L'`');
    QChar aigu(L'´');
    QChar circonflexe(L'^');

    QTest::newRow("no shift, no accent")            << false << QChar() << "a";
    QTest::newRow("no shift, l'accent grave")       << false << grave << QString(L'à');
    QTest::newRow("no shift, l'accent aigu")        << false << aigu << QString(L'á');
    QTest::newRow("no shift, l'accent circonflexe") << false << circonflexe << QString(L'â');
    QTest::newRow("shift, no accent")               << true  << QChar() << "A";
    QTest::newRow("shift, l'accent grave")          << true  << grave << QString(L'À');
}

void Ut_KeyButton::testSetModifier()
{
    QFETCH(bool, shift);
    QFETCH(QChar, accent);
    QFETCH(QString, expectedLabel);

    subject->setModifiers(shift, accent);
    QCOMPARE(subject->label(), expectedLabel);
}

void Ut_KeyButton::testKey()
{
    QCOMPARE(&subject->key(), dataKey);
}

void Ut_KeyButton::testBinding()
{
    bool shift = false;
    subject->setModifiers(shift);
    QCOMPARE(&subject->binding(), dataKey->binding(shift));

    shift = true;
    subject->setModifiers(shift);
    QCOMPARE(&subject->binding(), dataKey->binding(shift));
}

void Ut_KeyButton::testIsDead()
{
    MImKeyModel *key = new MImKeyModel;
    MImKeyBinding *binding = new MImKeyBinding;
    key->bindings[MImKeyModel::NoShift] = binding;

    MImAbstractKey *subject = new MImKey(*key, *style, *parent);

    for (int i = 0; i < 2; ++i) {
        bool isDead = (i != 0);
        binding->dead = isDead;
        QCOMPARE(subject->isDeadKey(), isDead);
    }

    delete subject;
    delete key;
}

void Ut_KeyButton::testTouchPointCount_data()
{
    QTest::addColumn<int>("initialCount");
    QTest::addColumn< QList<DirectionPair> >("countDirectionList");
    QTest::addColumn<int>("expectedCount");
    QTest::addColumn<MImAbstractKey::ButtonState>("expectedButtonState");

    QTest::newRow("increase and press button")
        << 0 << (QList<DirectionPair>() << DirectionPair(Up, true))
        << 1 << MImAbstractKey::Pressed;

    QTest::newRow("decrease and release button")
        << 1 << (QList<DirectionPair>() << DirectionPair(Down, true))
        << 0 << MImAbstractKey::Normal;

    QTest::newRow("try to take more than possible")
        << 0 << (QList<DirectionPair>() << DirectionPair(Up, true)
                                        << DirectionPair(Down, true)
                                        << DirectionPair(Down, false))
        << 0 << MImAbstractKey::Normal;

    QTest::newRow("try to take more than possible, again")
        << 0 << (QList<DirectionPair>() << DirectionPair(Up, true)
                                        << DirectionPair(Down, true)
                                        << DirectionPair(Down, false)
                                        << DirectionPair(Up, true))
        << 1 << MImAbstractKey::Pressed;

    QTest::newRow("go to the limit")
        << MImKey::touchPointLimit() << (QList<DirectionPair>() << DirectionPair(Up, false))
        << MImKey::touchPointLimit() << MImAbstractKey::Pressed;

    QTest::newRow("go to the limit, again")
        << MImKey::touchPointLimit() << (QList<DirectionPair>() << DirectionPair(Up, false)
                                         << DirectionPair(Down, true)
                                         << DirectionPair(Down, true))
        << MImKey::touchPointLimit() - 2 << MImAbstractKey::Pressed;
}

void Ut_KeyButton::testTouchPointCount()
{
    QFETCH(int, initialCount);
    QFETCH(QList<DirectionPair>, countDirectionList);
    QFETCH(int, expectedCount);
    QFETCH(MImAbstractKey::ButtonState, expectedButtonState);

    for (int idx = 0; idx < initialCount; ++idx) {
        subject->increaseTouchPointCount();
    }

    QCOMPARE(subject->touchPointCount(), initialCount);

    foreach(const DirectionPair &pair, countDirectionList) {
        switch (pair.first) {
        case Up:
            QCOMPARE(subject->increaseTouchPointCount(), pair.second);
            break;

        case Down:
            QCOMPARE(subject->decreaseTouchPointCount(), pair.second);
            break;
        }
    }

    QCOMPARE(subject->touchPointCount(), expectedCount);
    QCOMPARE(subject->state(), expectedButtonState);
}

MImKeyModel *Ut_KeyButton::createDataKey()
{
    MImKeyModel *key = new MImKeyModel;

    MImKeyBinding *binding1 = new MImKeyBinding;
    binding1->keyLabel = "a";
    binding1->dead = false;
    binding1->accents = "`´^¨";
    binding1->accented_labels = QString(L'à') + L'á' + L'á' + L'â' + L'ä';

    MImKeyBinding *binding2 = new MImKeyBinding;
    binding2->keyLabel = "A";
    binding2->dead = false;
    binding2->accents = "`´^¨";
    binding2->accented_labels = QString(L'À') + L'Á' + L'Â' + L'Ä';

    key->bindings[MImKeyModel::NoShift] = binding1;
    key->bindings[MImKeyModel::Shift] = binding2;

    return key;
}

QTEST_APPLESS_MAIN(Ut_KeyButton);