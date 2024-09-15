#pragma once

#include <common.h>

#include <QAbstractListModel>

namespace lsystem {

struct ConfigNameKind
{
	QString configName;
	bool fromUser = false;
};

class ConfigList : public QAbstractListModel
{
	Q_OBJECT
public:
	ConfigList(QWidget * parentWidget = nullptr);

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role) const override;

	bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;

public:
	bool storeConfig(const common::ConfigSet & configSet);
	bool deleteConfig(const QString & configName);

	ConfigNameKind getConfigNameKindByIndex(const QModelIndex & index);
	common::ConfigSet getConfigByIndex(const QModelIndex & index);

public slots:
	void newPreAndUserConfigs(const common::ConfigMap & preConfigs, const common::ConfigMap & userConfigs);

signals:
	void configMapUpdated(const common::ConfigMap & configMap);

private:
	void updateConfigNames();

	auto getRow(const QModelIndex & index) const;
	auto getRow(const QModelIndex & index);
	void allDataChanged();

private:
	common::ConfigMap preConfigs;
	common::ConfigMap userConfigs;
	QStringList configNames;

	QWidget * const parentWidget;
};

}
