#pragma once

#include <QtCore>
#include <QStyledItemDelegate>

/// An item delegate for a QTableView, which auto-updates the content while editing

// https://stackoverflow.com/questions/10081033/qt-signal-while-a-qtableview-item-data-is-being-edited-instead-of-after-edit-is
class TableItemDelegateAutoUpdate : public QStyledItemDelegate
{
public:
	TableItemDelegateAutoUpdate(QObject* parent = nullptr);
	QWidget* createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};
