#include "configfilestore.h"

#include <jsonkeys.h>
#include <util/print.h>

using namespace lsystem::common;
using namespace lsystem::constants;
using namespace util;

namespace {

const constexpr char * UserConfigFile = "config.json";
const constexpr char * PredefinedConfigFile = ":/data/predefined-config.json";
}

namespace lsystem {

AppConfig::AppConfig(const QJsonObject & obj)
	: isNull(obj.find(JsonKeySettings) == obj.end() || obj.find("configs") == obj.end())
	, settings(obj[JsonKeySettings].toObject())
	, configMap(obj[JsonKeyConfigs].toObject())
{
}

QJsonObject AppConfig::toJson() const
{
	QJsonObject rv;
	rv[JsonKeySettings] = settings.toJson();
	rv[JsonKeyConfigs] = configMap.toJson();
	return rv;
}

// ------------------------------------------------------------------------------------------------------------------------

void ConfigFileStore::loadConfig()
{
	ConfigMap preConfigs;

	{
		AppConfig appConfig = getConfigFromFile(PredefinedConfigFile);
		if (!appConfig.isNull) {
			preConfigs = appConfig.configMap;
			currentConfig.settings = appConfig.settings;
		} else {
			emit showError(printStr("predefined config file %1 could not be loaded", QFileInfo(PredefinedConfigFile)));
		}
	}

	if (QFile::exists(UserConfigFile)) {
		AppConfig appConfig = getConfigFromFile(UserConfigFile);
		if (!appConfig.isNull) {
			currentConfig = appConfig;
		} else {
			emit showError(printStr("user config file %1 could not be loaded", QFileInfo(UserConfigFile)));
		}
	} else {
		saveCurrentConfig();
	}

	emit loadedPreAndUserConfigs(preConfigs, currentConfig.configMap);
	settingsUpdated();
}

void ConfigFileStore::saveCurrentConfig()
{
	QFile file(UserConfigFile);
	if (!file.open(QFile::WriteOnly)) {
		emit showError(printStr("could not write to config file %1", QFileInfo(file)));
		return;
	}
	QJsonDocument doc(currentConfig.toJson());
	file.write(doc.toJson());
	file.close();
}

void ConfigFileStore::newConfigMap(const common::ConfigMap & configMap)
{
	currentConfig.configMap = configMap;
	saveCurrentConfig();
}

AppSettings ConfigFileStore::getSettings() const
{
	return currentConfig.settings;
}

void ConfigFileStore::saveSettings(const common::AppSettings & settings)
{
	currentConfig.settings = settings;
	saveCurrentConfig();
	settingsUpdated();
}

AppConfig ConfigFileStore::getConfigFromFile(const QString & filePath)
{
	QFile file(filePath);
	if (!file.exists()) {
		return AppConfig();
	}
	file.open(QFile::ReadOnly);
	const QByteArray content = file.readAll();

	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(content, &err);

	if (err.error != QJsonParseError::NoError) {
		return AppConfig();
	}

	return AppConfig(doc.object());

}

void ConfigFileStore::settingsUpdated()
{
	emit newStackSize(currentConfig.settings.maxStackSize);
}


}
