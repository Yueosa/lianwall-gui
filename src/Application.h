#pragma once

#include <QObject>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>

class ConfigManager;

/**
 * @brief 应用程序主类 (MVP 版)
 * 
 * 精简架构，只管理必要组件
 */
class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    int exec();

    ConfigManager* configManager() const { return m_configManager; }

public slots:
    void showMainWindow();
    void hideMainWindow();
    void quit();
    void switchLanguage(const QString &lang);

signals:
    void aboutToQuit();

private:
    void initComponents();
    void registerQmlTypes();
    void loadTranslations();

    QApplication *m_app;
    QQmlApplicationEngine *m_engine;
    QTranslator *m_translator;
    ConfigManager *m_configManager;
};
