#include "configfilestore.h"

using namespace lsystem::common;

namespace {

const constexpr char * UserConfigFile = "config.json";
const constexpr char * PredefinedConfigFile = ":/data/predefined-config.json";

}

namespace lsystem {

AppConfig::AppConfig(const QJsonObject & obj)
	: isNull(obj.find("settings") == obj.end() || obj.find("configs") == obj.end()),
	  settings(obj["settings"].toObject()),
	  configMap(obj["configs"].toObject())
{
}

QJsonObject AppConfig::toJson() const
{
	QJsonObject rv;
	rv["settings"] = settings.toJson();
	rv["configs"] = configMap.toJson();
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
			showError(QString("predefined config file %1 could not be loaded")
					.arg(QFileInfo(PredefinedConfigFile).absoluteFilePath()));
		}
	}

	if (QFile::exists(UserConfigFile)) {
		AppConfig appConfig = getConfigFromFile(UserConfigFile);
		if (!appConfig.isNull) {
			currentConfig = appConfig;
		} else {
			showError(QString("user config file %1 could not be loaded").arg(QFileInfo(UserConfigFile).absoluteFilePath()));
		}
	} else {
		saveCurrentConfig();
	}

	emit loadedPreAndUserConfigs(preConfigs, currentConfig.configMap);
	emit loadedAppSettings(currentConfig.settings);
}

void ConfigFileStore::saveCurrentConfig()
{
	QFile file(UserConfigFile);
	if (!file.open(QFile::WriteOnly)) {
		emit showError(QString("could not write to config file %1").arg(QFileInfo(file).absoluteFilePath()));
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


}
