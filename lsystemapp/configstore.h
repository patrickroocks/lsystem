#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <common.h>

#include <QAbstractListModel>

namespace lsystem {

struct ConfigNameKind
{
	QString configName;
	bool fromUser = false;
};

class ConfigStore : public QAbstractListModel
{
	Q_OBJECT
public:
	ConfigStore(QWidget * parentWidget = nullptr);
	~ConfigStore() {}

	bool loadConfig();

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role) const override;

	bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;

public:
	bool storeConfig(const QString & configName, const common::ConfigSet & configSet);
	bool deleteConfig(const QString & configName);

	ConfigNameKind getConfigNameKindByIndex(const QModelIndex & index);
	common::ConfigSet getConfigByIndex(const QModelIndex & index);

private:
	void updateConfigNames();
	void storeUserConfigsInFile();

	using ConfigMap = QMap<QString, common::ConfigSet>;
	ConfigMap getConfigsFromFile(const QString & filePath, bool & ok);

	auto getRow(const QModelIndex & index) const;
	auto getRow(const QModelIndex & index);
	void allDataChanged();

private:
	ConfigMap preConfigs;
	ConfigMap userConfigs;
	QStringList configNames;

	QWidget * parentWidget;
};

}

#endif // CONFIGSTORE_H
