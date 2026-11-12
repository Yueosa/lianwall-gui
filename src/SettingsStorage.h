#pragma once
#include <QString>

namespace LianwallGui::Paths {
/// Copy legacy GUI preferences only when the destination is absent; never remove the source.
bool migrateGuiSettings(const QString &source, const QString &destination);
QString guiSettingsFile();
}
