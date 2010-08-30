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



#ifndef LAYOUTDATA_H
#define LAYOUTDATA_H

#include <MNamespace>
#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

class VKBDataKey;

/*!
 * \brief LayoutSection represents a named area of keys in a layout
 */
class LayoutSection
{
    Q_DISABLE_COPY(LayoutSection)

public:
    enum Type {
        //! The section uses sloppy mode by default. This is the default value.
        Sloppy = 0,
        //! The section uses discrete layout
        NonSloppy
    };

    ~LayoutSection();
    LayoutSection();

    //! \return section name
    const QString &name() const;

    //! \return section type
    Type type() const;

    /*!
     * \brief Get the maximum columns in this section
     * \return the maximum columns in this section
     */
    int maxColumns() const;

    /*!
     * \brief Get the maximum width in this section, but in normalized units
     * \return the maxium normalized width in this section
     */
    qreal maxNormalizedWidth() const;

    /*!
     * \brief Get the maximum number of rows in this section
     * \return the maximum number of rows in this section
     */
    int rowCount() const;

    /*!
     * \brief Get the number of columns in the specified section and row
     * \param row number
     * \return the number of columns in the specified section and row
     */
    int columnsAt(int row) const;

    //! \brief Number of keys in the section
    int keyCount() const;

    //! \brief Get indices with layout spacers for given row
    //! \param row number
    QList<int> spacerIndices(int row) const;


    /*!
     * \brief Get key at specified row, column and section
     * \param row
     * \param column
     * \return key
     */
    VKBDataKey *vkbKey(int row, int column) const;

    /*!
     * \brief Get horizontal alignment of this section.
     */
    Qt::Alignment horizontalAlignment() const;

    /*!
     * \brief Get vertical alignment of this section.
     */
    Qt::Alignment verticalAlignment() const;

private:
    bool isInvalidRow(int row) const;
    bool isInvalidCell(int row, int column) const;

    struct Row {
        Row()
            : normalizedWidth(0.0)
        {}

        ~Row();

        QList<VKBDataKey *> keys;
        qreal normalizedWidth;

        // Index of a spacer refers to right side of a key;
        // -1 means spacer before first key.
        QList<int> spacerIndices;
    };

    int mMaxColumns;
    qreal mMaxNormalizedWidth;
    int maxRows;
    bool movable;
    Qt::Alignment m_verticalAlignment;
    Qt::Alignment m_horizontalAlignment;
    QString sectionName;
    Type sectionType;
    QList<Row *> rows;

    friend class KeyboardData;
    friend class LayoutModel;
    friend class ParseParameters;
};

/*!
 * \class LayoutData
 * \brief LayoutData represents a keyboard layout of certain type and orientation
 */
class LayoutData
{
    Q_DISABLE_COPY(LayoutData)

public:
    typedef QSharedPointer<LayoutSection> SharedLayoutSection;

    //! Type of layout
    enum LayoutType {
        General = 0,
        Url,
        Email,
        Number,
        PhoneNumber,
        Common,
        NumLayoutTypes
    };

    static const QString mainSection;
    static const QString functionkeySection;
    static const QString symbolsSymSection;

    //! \brief Constructor
    LayoutData();

    //! \brief Destructor
    virtual ~LayoutData();

    //! \return the number of sections in this layout
    int numSections() const;

    //! \return a section by section index
    SharedLayoutSection section(int index) const;

    /*! \return a section by section name.  if there are several
     * identically named sections, it is unspecified which one is
     * returned
     */
    SharedLayoutSection section(const QString &name) const;

    //! \return layout type
    LayoutType type() const;

    //! \return layout orientation
    M::Orientation orientation() const;

protected:
    M::Orientation layoutOrientation;
    LayoutType layoutType;
    //! this is the top level data structure of a layout
    QList<SharedLayoutSection> sections;
    //! we keep sections also in a hash table for fast name based lookup
    QHash<QString, SharedLayoutSection> sectionMap;

    friend class KeyboardData;
};

#endif //LAYOUTMODEL_H
