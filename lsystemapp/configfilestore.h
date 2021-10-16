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

signals:
	void loadedPreAndUserConfigs(const common::ConfigMap & preConfigs, const common::ConfigMap & userConfigs);
	void loadedAppSettings(const common::AppSettings & settings);
	void showError(const QString & errorText);

private:
	void saveCurrentConfig();
	static AppConfig getConfigFromFile(const QString & filePath);

private:
	AppConfig currentConfig;
};

}

#endif // CONFIGFILESTORE_H
