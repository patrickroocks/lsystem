#include "configstore.h"

#include <util/qt-cont-utils.h>

#include <QMessageBox>

using namespace lsystem::common;
using namespace util;

namespace {

const QString UserConfigPrefix = "[user] ";
const QString UserConfigFile = "userconfigs.json";
const QString PredefinedConfigFile = ":/data/predefined-configs.json";

}

namespace lsystem {

auto ConfigStore::getRow(const QModelIndex & index) const
{
	return configNames.begin() + index.row();
}

auto ConfigStore::getRow(const QModelIndex & index)
{
	return configNames.begin() + index.row();
}

void ConfigStore::allDataChanged()
{
	dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

ConfigStore::ConfigStore(QWidget * parentWidget)
	: parentWidget(parentWidget)
{
}

bool ConfigStore::loadConfig()
{
	bool okPreDef = false;
	preConfigs = getConfigsFromFile(PredefinedConfigFile, okPreDef);

	bool okUser = false;
	userConfigs = getConfigsFromFile(UserConfigFile, okUser);
	if (okPreDef || okUser) {
		updateConfigNames();
		allDataChanged();
	}
	return okPreDef && okUser;
}

int ConfigStore::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return configNames.size();
}


QVariant ConfigStore::data(const QModelIndex & index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0) {

		if (index.row() < configNames.size()) {
			return *getRow(index);
		}
	}

	return QVariant();
}

bool ConfigStore::insertRows(int row, int count, const QModelIndex & parent)
{
	Q_UNUSED(count);
	beginInsertRows(parent, row, row);
	endInsertRows();
	return true;
}

bool ConfigStore::removeRows(int row, int count, const QModelIndex & parent)
{
	Q_UNUSED(count);
	beginRemoveRows(parent, row, row);
	endRemoveRows();
	return true;
}

bool ConfigStore::storeConfig(const QString & configName, const common::ConfigSet & configSet)
{
	if (configName.isEmpty()) {
		QMessageBox::information(parentWidget, "Error", "Config name must not be empty");
		return false;
	}

	bool overwritten = false;
	if (userConfigs.contains(configName)) {
		QMessageBox::StandardButton result = QMessageBox::question(parentWidget, "Overwrite?",
				QString("Overwrite existing config with name '%1'?").arg(configName));
		if (result == QMessageBox::No) return false;
		overwritten = true;
	}

	userConfigs[configName] = configSet;
	if (!overwritten) {
		updateConfigNames();
		insertRow(configNames.size() - 1);
		allDataChanged();
	}
	storeUserConfigsInFile();

	return false;
}

bool ConfigStore::deleteConfig(const QString & configName)
{
	if (userConfigs.remove(configName) == 0) return false;

	updateConfigNames();
	removeRow(configNames.size());
	allDataChanged();
	storeUserConfigsInFile();
	return true;
}

ConfigNameKind ConfigStore::getConfigNameKindByIndex(const QModelIndex & index)
{
	if (!index.isValid()) return ConfigNameKind();

	const QString & configName = *getRow(index);

	ConfigNameKind rv;
	if (configName.startsWith(UserConfigPrefix)) {
		rv.configName = configName.mid(UserConfigPrefix.size());
		rv.fromUser = true;
	} else {
		rv.configName = configName;
		rv.fromUser = false;
	}
	return rv;
}

common::ConfigSet ConfigStore::getConfigByIndex(const QModelIndex & index)
{
	ConfigNameKind configNameKind = getConfigNameKindByIndex(index);

	common::ConfigSet rv;

	auto checkValid = [&]() {
		rv.valid = !rv.definitions.isEmpty();
	};

	if (configNameKind.fromUser) {
		if (userConfigs.contains(configNameKind.configName)) {
			rv = userConfigs[configNameKind.configName];
			checkValid();
		}
	} else {
		if (preConfigs.contains(configNameKind.configName)) {
			rv = preConfigs[configNameKind.configName];
			checkValid();
		}
	}

	return rv;
}

void ConfigStore::updateConfigNames()
{
	configNames.clear();
	for (const auto & [key, value] : KeyVal(preConfigs))  configNames << key;
	for (const auto & [key, value] : KeyVal(userConfigs)) configNames << UserConfigPrefix + key;
}

void ConfigStore::storeUserConfigsInFile()
{
	QJsonObject configs;
	for (const auto & [key, value] : KeyVal(userConfigs)) {
		configs[key] = value.toJson();
	}
	QJsonDocument doc(configs);

	QFile file(UserConfigFile);
	file.open(QFile::WriteOnly);
	file.write(doc.toJson());
	file.close();
}

ConfigStore::ConfigMap ConfigStore::getConfigsFromFile(const QString & filePath, bool & ok)
{
	QFile file(filePath);
	if (!file.exists()) {
		ok = true;
		return ConfigStore::ConfigMap();
	}
	file.open(QFile::ReadOnly);
	const QByteArray content = file.readAll();

	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(content, &err);

	if (err.error != QJsonParseError::NoError) {
		ok = false;
		return ConfigStore::ConfigMap();
	}

	ConfigStore::ConfigMap rv;
	QVariantMap tmpMap = doc.object().toVariantMap();
	for (const auto & [key, value] : KeyVal(tmpMap)) {
		rv[key] = ConfigSet(value.toJsonObject());
	}

	ok = true;
	return rv;
}

}
