#include "tableitemdelegate.h"

#include <QLineEdit>

TableItemDelegateAutoUpdate::TableItemDelegateAutoUpdate(QObject * parent)
	: QStyledItemDelegate(parent)
	, mapper(new QSignalMapper(this))
{
	connect(mapper, SIGNAL(mapped(QWidget*)), SIGNAL(commitData(QWidget*)));
}

QWidget * TableItemDelegateAutoUpdate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
	if (qobject_cast<QLineEdit*>(editor)) {
		connect(editor, SIGNAL(textChanged(QString)), mapper, SLOT(map()));
		mapper->setMapping(editor, editor);
	}
	return editor;
}
