#include "definitionmodel.h"

#include <QBrush>
#include <QColorDialog>

namespace  {

const int MaxNumDefinitions = 5;

}

using namespace lsystem::common;

namespace lsystem {

DefinitionModel::DefinitionModel(QWidget * parent)
	: parent(parent)
{
	Definition & defaultDef = definitions['A'];
	defaultDef.command = "A+A";
	defaultDef.color = qRgb(0, 0, 0);
	defaultDef.paint = true;
	checkForNewStartSymbol();
}

auto DefinitionModel::getRow(const QModelIndex & index) const
{
	return definitions.begin() + index.row();
}

auto DefinitionModel::getRow(const QModelIndex & index)
{
	return definitions.begin() + index.row();
}

void DefinitionModel::checkForNewStartSymbol()
{
	const char tmpStartSymbol = definitions.firstKey();
	if (tmpStartSymbol != startSymbol) {
		startSymbol = tmpStartSymbol;
		emit newStartSymbol(QString(1, startSymbol));
	}
}

int DefinitionModel::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return definitions.size();
}

int DefinitionModel::columnCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return 4;
}

QVariant DefinitionModel::data(const QModelIndex & index, int role) const
{
	if (role == Qt::DisplayRole || role == Qt::EditRole) {

		if (index.column() == 0) return QString(getRow(index).key());
		else if (index.column() == 1) return getRow(index).value().command;

	} else if (role == Qt::BackgroundRole) {
		if (index.column() == 2) return QBrush(getRow(index).value().color);

	} else if (role == Qt::CheckStateRole) {
		if (index.column() == 3) return getRow(index)->paint ? Qt::Checked : Qt::Unchecked;

	}


	return QVariant();
}

bool DefinitionModel::insertRows(int row, int count, const QModelIndex & parent)
{
	Q_UNUSED(count);
	beginInsertRows(parent, row, row + count - 1);
	endInsertRows();
	return true;
}

bool DefinitionModel::removeRows(int row, int count, const QModelIndex & parent)
{
	Q_UNUSED(count);
	beginRemoveRows(parent, row, row + count - 1);
	endRemoveRows();
	return true;
}

Qt::ItemFlags DefinitionModel::flags(const QModelIndex & index) const
{
	Qt::ItemFlags rv = QAbstractTableModel::flags(index);
	if      (index.column() <= 1) rv |= Qt::ItemIsEditable;
	else if (index.column() == 3) rv |= Qt::ItemIsUserCheckable;
	return rv;
}

QVariant DefinitionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
		case 0: return QString("Literal");
		case 1: return QString("Command");
		case 2: return QString("Color");
		case 3: return QString("Paint");
		}

	}
	return QVariant();
}

bool DefinitionModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!checkIndex(index)) return false;

	auto itRow = getRow(index);

	if (role == Qt::EditRole) {
		if (index.column() == 0) {
			const QString literalStr = value.toString();
			if (literalStr.length() != 1) {
				emit showError(QString("Literal '%1' is not allowed, expected one char [A-Z]").arg(literalStr));
				return false;
			}
			const char newChar = value.toString()[0].toLatin1();
			if (newChar < 'A' || newChar > 'Z')  {
				emit showError(QString("Literal '%1' is not allowed, expected one char [A-Z]").arg(literalStr));
				return false;
			}
			const QSet<char> currentKeys = definitions.keys().toSet();
			if (currentKeys.contains(newChar)) {
				emit showError(QString("Literal '%1' must not be used twice").arg(literalStr));
				return false;
			}
			const char oldChar = itRow.key();
			definitions[newChar] = definitions[oldChar];
			definitions.remove(oldChar);
			checkForNewStartSymbol();
			emit edited();
			return true;
		} else if (index.column() == 1) {
			if (itRow.value().command != value.toString()) {
				itRow.value().command = value.toString();
				emit edited();
			}
			return true;
		}
	} else if (role == Qt::CheckStateRole) {
		if (index.column() == 3) {
			const bool newChecked = (Qt::CheckState)value.toInt() == Qt::Checked;
			if (newChecked != itRow->paint) {
				itRow->paint = newChecked;
				emit edited();
			}
			return true;
		}
	}
	return false;
}

void DefinitionModel::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	Q_UNUSED(deselected);
	if (selected.indexes().size() != 1) return;
	QModelIndex ind = selected.indexes().first();
	if (ind.column() != 2) return;
	auto row = getRow(ind);
	bool ok;
	QColorDialog diag;
	const QRgb newColor = QColorDialog::getRgba(row->color, &ok, parent);
	if (newColor != row.value().color) {
		row.value().color = newColor;
	}
	emit deselect();
}

bool DefinitionModel::add()
{
	if (definitions.size() < MaxNumDefinitions) {
		const int addRowNum = definitions.size();

		const QSet<char> currentKeys = definitions.keys().toSet();
		char nextChar = definitions.lastKey();
		while (true) {
			nextChar++;
			if (nextChar == 'Z') nextChar = 'A';
			if (!currentKeys.contains(nextChar)) break;
		}
		definitions[nextChar].color = qRgb(0, 0, 0);
		insertRow(addRowNum);
		return true;
	}
	return false;
}

bool DefinitionModel::remove()
{
	if (definitions.size() > 1) {
		const int removeRowNum = definitions.size() - 1;
		auto it = definitions.begin() + removeRowNum;
		definitions.erase(it);
		removeRow(removeRowNum);
		return true;
	}
	return false;
}

void DefinitionModel::setDefinitions(const Definitions & newDefinitions)
{
	int sizeDiff = newDefinitions.size() - definitions.size();
	definitions = newDefinitions;

	if (sizeDiff < 0) removeRows(0, -sizeDiff);
	else if (sizeDiff > 0) insertRows(0, sizeDiff);
	dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, columnCount() - 1));
	checkForNewStartSymbol();
}


}
