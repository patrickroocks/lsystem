#ifndef DEFINITIONMODEL_H
#define DEFINITIONMODEL_H

#include <common.h>

#include <QAbstractTableModel>

namespace lsystem {

class DefinitionModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	DefinitionModel(QWidget * parent = nullptr);

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
	Qt::ItemFlags flags(const QModelIndex & index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
	bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;

	void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
	bool add();
	bool remove();

	common::Definitions getDefinitions() const { return definitions; }
	void setDefinitions(const common::Definitions & newDefinitions);

signals:
	void deselect();
	void newStartSymbol(const QString & startSymbol);
	void showError(const QString & errorText);
	void edited();

private:
	auto getRow(const QModelIndex & index) const;
	auto getRow(const QModelIndex & index);
	void checkForNewStartSymbol();

private:
	common::Definitions definitions;
	QWidget * parent;
	char startSymbol = '\0';
};

}

#endif // DEFINITIONMODEL_H
