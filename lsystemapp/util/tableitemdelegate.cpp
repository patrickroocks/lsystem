#include "tableitemdelegate.h"

#include <QLineEdit>

TableItemDelegateAutoUpdate::TableItemDelegateAutoUpdate(QObject * parent)
	: QStyledItemDelegate(parent)
{
}

QWidget * TableItemDelegateAutoUpdate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
	if (QLineEdit* castedEditor = qobject_cast<QLineEdit*>(editor)) {

		// createEditor must be const, such that it is called. commitData is non-const
		connect(castedEditor, &QLineEdit::textEdited, [&](const QString&) { const_cast<TableItemDelegateAutoUpdate*>(this)->commitData(editor); });
	}
	return editor;
}
