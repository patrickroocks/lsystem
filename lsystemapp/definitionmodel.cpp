#include "definitionmodel.h"

#include <util/print.h>

#include <QBrush>
#include <QColorDialog>

namespace {

const int MaxNumDefinitions = 5;

}

using namespace lsystem::common;
using namespace util;

namespace lsystem {

DefinitionModel::DefinitionModel(QWidget * parent)
	: parent(parent)
{
	Definition defaultDef;
	defaultDef.literal = 'A';
	defaultDef.command = "A+A";
	defaultDef.color = QColor(0, 0, 0);
	defaultDef.paint = true;
	definitions << defaultDef;
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
	const char tmpStartSymbol = definitions.first().literal;
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
		if (index.column() == 0) return QString(getRow(index)->literal);
		else if (index.column() == 1) return getRow(index)->command;

	} else if (role == Qt::BackgroundRole) {
		if (index.column() == 2) return QBrush(getRow(index)->color);

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
	if (index.isValid()) {
		Qt::ItemFlags rv = QAbstractTableModel::flags(index);
		if      (index.column() <= 1) rv |= Qt::ItemIsEditable;
		else if (index.column() == 3) rv |= Qt::ItemIsUserCheckable;
		return rv | Qt::ItemIsDragEnabled;
	} else {
		return Qt::ItemIsDropEnabled;
	}
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
			itRow->literal = newChar;
			checkForNewStartSymbol();
			emit edited();
			return true;
		} else if (index.column() == 1) {
			if (itRow->command != value.toString()) {
				itRow->command = value.toString();
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

bool DefinitionModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
	ALL_UNUSED(column, parent);

	if (action != Qt::MoveAction) return false;

	const QStringList & mimeTypesRef = mimeTypes();
	if (mimeTypesRef.isEmpty()) return false;
	const QByteArray encoded = data->data(mimeTypesRef.first());
	QDataStream stream(encoded);

	if (stream.atEnd()) return false; // no data

	int srcRow, srcCol;
	QMap<int,  QVariant> roleDataMap;
	stream >> srcRow >> srcCol >> roleDataMap;

	// too much data? there should be only one single cell selectable
	if (!stream.atEnd()) return false;

	// do the move
	const QModelIndex srcIndex = createIndex(srcRow, srcCol);
	if (row == srcRow || row == srcRow + 1) {
		emit showError(printStr("Cannot move definition %1: target and destination are identical", getRow(srcIndex)->literal));
		return false;
	}
	definitions.insert(row, *getRow(srcIndex));
	if (srcRow > row) { // delete below insertion
		++srcRow;
	} else { // delete above insertion
		--row;
	}
	definitions.removeAt(srcRow);
	emit dataChanged(createIndex(qMin(row, srcRow), 0), createIndex(qMax(row, srcRow), columnCount() - 1));
	checkForNewStartSymbol();
	return true;
}

void DefinitionModel::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	Q_UNUSED(deselected);
	if (selected.indexes().size() != 1) return;
	QModelIndex ind = selected.indexes().first();
	if (ind.column() != 2) return;
	auto row = getRow(ind);
	QColorDialog diag;
	const QColor newColor = QColorDialog::getColor(row->color, parent);
	if (newColor.isValid() && newColor != row->color) {
		row->color = newColor;
	}
	emit deselect();
}

bool DefinitionModel::add()
{
	if (definitions.size() < MaxNumDefinitions) {
		const int addRowNum = definitions.size();

		QSet<char> currentLiterals;
		for (const Definition & def : qAsConst(definitions)) currentLiterals << def.literal;
		char nextChar = definitions.last().literal;
		while (true) {
			nextChar++;
			if (nextChar == 'Z') nextChar = 'A';
			if (!currentLiterals.contains(nextChar)) break;
		}
		Definition newDefinition;
		newDefinition.literal = nextChar;
		newDefinition.color = QColor(0, 0, 0);
		newDefinition.paint = true;
		definitions << newDefinition;
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
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, columnCount() - 1));
	checkForNewStartSymbol();
}

}
