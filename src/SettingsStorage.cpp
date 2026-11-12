#include "SettingsStorage.h"
#include "Constants.h"
#include <QSaveFile>
#include <QFileInfo>
#include <QDebug>

bool LianwallGui::Paths::migrateGuiSettings(const QString &source, const QString &destination)
{
    if (QFileInfo::exists(destination) || QFileInfo(destination).isSymLink())
        return true;
    QFile previous(source);
    if (!previous.open(QIODevice::ReadOnly))
        return false;
    const QByteArray content = previous.readAll();
    if (previous.error() != QFile::NoError || !QDir().mkpath(QFileInfo(destination).absolutePath()))
        return false;
    QSaveFile output(destination);
    return output.open(QIODevice::WriteOnly) && output.write(content) == content.size() && output.commit();
}

QString LianwallGui::Paths::guiSettingsFile()
{
    const QString current = configDir() + QStringLiteral("/gui.conf");
    const QString legacy = QDir::homePath() + QStringLiteral("/.config/lianwall/gui.conf");
    if (current != legacy && !QFileInfo::exists(current) && QFileInfo::exists(legacy)) {
        if (!migrateGuiSettings(legacy, current)) {
            qWarning() << "[SettingsStorage] GUI preference migration failed; using legacy file:" << legacy;
            return legacy;
        }
    }
    return current;
}
