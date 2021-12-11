#ifndef CONFIGFILESTORE_H
#define CONFIGFILESTORE_H

#include <common.h>

#include <QAbstractListModel>

namespace lsystem {

struct AppConfig
{
	bool isNull = true;
	common::AppSettings settings;
	common::ConfigMap configMap;
	AppConfig() = default;
	explicit AppConfig(const QJsonObject & obj);
	QJsonObject toJson() const;
};

class ConfigFileStore : public QObject
{
	Q_OBJECT
public:
	void loadConfig();

public slots:
	void newConfigMap(const common::ConfigMap & configMap);
	common::AppSettings getSettings() const;
	void saveSettings(const common::AppSettings& settings);

signals:
	void loadedPreAndUserConfigs(const common::ConfigMap & preConfigs, const common::ConfigMap & userConfigs);
	void newStackSize(int newMaxStackSize);
	void showError(const QString & errorText);

private:
	void saveCurrentConfig();
	static AppConfig getConfigFromFile(const QString & filePath);
	void settingsUpdated();

private:
	AppConfig currentConfig;
};

}

#endif // CONFIGFILESTORE_H
