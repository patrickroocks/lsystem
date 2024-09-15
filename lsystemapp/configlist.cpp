#include "configlist.h"

#include <util/qtcontutils.h>

#include <QMessageBox>

using namespace lsystem::common;
using namespace util;

namespace {

constexpr const char * UserConfigPrefix = "[user] ";

}

namespace lsystem {

auto ConfigList::getRow(const QModelIndex & index) const
{
	return configNames.begin() + index.row();
}

auto ConfigList::getRow(const QModelIndex & index)
{
	return configNames.begin() + index.row();
}

void ConfigList::allDataChanged()
{
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

ConfigList::ConfigList(QWidget * parentWidget)
	: parentWidget(parentWidget)
{
}

void ConfigList::newPreAndUserConfigs(const common::ConfigMap & preConfigs, const common::ConfigMap & userConfigs)
{
	this->preConfigs = preConfigs;
	this->userConfigs = userConfigs;
	updateConfigNames();
	allDataChanged();
}

int ConfigList::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return configNames.size();
}

QVariant ConfigList::data(const QModelIndex & index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0) {
		if (index.row() < configNames.size()) {
			return *getRow(index);
		}
	}

	return QVariant();
}

bool ConfigList::insertRows(int row, int count, const QModelIndex & parent)
{
	Q_UNUSED(count);
	beginInsertRows(parent, row, row);
	endInsertRows();
	return true;
}

bool ConfigList::removeRows(int row, int count, const QModelIndex & parent)
{
	Q_UNUSED(count);
	beginRemoveRows(parent, row, row);
	endRemoveRows();
	return true;
}

bool ConfigList::storeConfig(const common::ConfigSet & configSet)
{
	if (configSet.name.isEmpty()) {
		QMessageBox::information(parentWidget, "Error", "Config name must not be empty");
		return false;
	}

	bool overwritten = false;
	if (userConfigs.contains(configSet.name)) {
		const QMessageBox::StandardButton result
			= QMessageBox::question(parentWidget, "Overwrite?", QString("Overwrite existing config with name '%1'?").arg(configSet.name));
		if (result == QMessageBox::No) return false;
		overwritten = true;
	}

	userConfigs[configSet.name] = configSet;
	if (!overwritten) {
		updateConfigNames();
		insertRow(configNames.size() - 1);
		allDataChanged();
	}
	emit configMapUpdated(userConfigs);

	return false;
}
bool ConfigList::deleteConfig(const QString & configName)
{
	if (userConfigs.remove(configName) == 0) return false;

	updateConfigNames();
	removeRow(configNames.size());
	allDataChanged();
	emit configMapUpdated(userConfigs);
	return true;
}

ConfigNameKind ConfigList::getConfigNameKindByIndex(const QModelIndex & index)
{
	if (!index.isValid()) return ConfigNameKind();

	const QString & configName = *getRow(index);

	ConfigNameKind rv;
	if (configName.startsWith(UserConfigPrefix)) {
		rv.configName = configName.mid(strlen(UserConfigPrefix));
		rv.fromUser = true;
	} else {
		rv.configName = configName;
		rv.fromUser = false;
	}
	return rv;
}

common::ConfigSet ConfigList::getConfigByIndex(const QModelIndex & index)
{
	ConfigNameKind configNameKind = getConfigNameKindByIndex(index);

	bool found = false;
	common::ConfigSet rv;

	if (configNameKind.fromUser) {
		if (userConfigs.contains(configNameKind.configName)) {
			rv = userConfigs[configNameKind.configName];
			found = true;
		}
	} else {
		if (preConfigs.contains(configNameKind.configName)) {
			rv = preConfigs[configNameKind.configName];
			found = true;
		}
	}

	if (found) rv.valid = !rv.definitions.isEmpty();
	return rv;
}

void ConfigList::updateConfigNames()
{
	configNames.clear();
	for (const auto & [key, value] : KeyVal(preConfigs))  configNames << key;
	for (const auto & [key, value] : KeyVal(userConfigs)) configNames << UserConfigPrefix + key;
}

}
