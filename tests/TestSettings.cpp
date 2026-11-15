#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonValue>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

static void typeAscii(QWindow *window, const QString &text)
{
    for (const auto character : text) QTest::keyClick(window, character.toLatin1());
}

static QQuickItem *itemNamed(QQuickItem *root, const QString &name)
{
    if (root->objectName() == name) return root;
    for (auto *child : root->childItems())
        if (auto *found = itemNamed(child, name)) return found;
    return nullptr;
}

class SettingsFixture : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap document READ document NOTIFY documentChanged)
    Q_PROPERTY(QStringList pendingKeys READ pendingKeys CONSTANT)
    Q_PROPERTY(bool daemonConnected READ daemonConnected CONSTANT)
public:
    QVariantMap values, canonicalOverrides;
    QList<QPair<QString,QVariant>> writes;
    bool failNext = false;
    QVariantMap document() const { return values; }
    QStringList pendingKeys() const { return {}; }
    bool daemonConnected() const { return true; }
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &fallback) const { return values.value(key, fallback); }
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value)
    {
        writes.append({key, value});
        if (failNext) { failNext = false; emit writeFinished(key, false, "save failed"); return; }
        values.insert(key, canonicalOverrides.value(key, value)); emit documentChanged(); emit writeFinished(key, true, {}); emit documentChanged();
    }
signals:
    void documentChanged();
    void writeFinished(const QString &key, bool success, const QString &message);
};
class TestSettings : public QObject
{
    Q_OBJECT
    QTemporaryDir directory;
    QString components;
private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        components = directory.path() + "/qml/components";
        QVERIFY(QDir().mkpath(components));
        const QString source = QString::fromUtf8(UI_SOURCE_DIR);
        for (const auto &name : {"ArgumentsSetting.qml", "NumberSetting.qml", "TextSetting.qml", "SettingRow.qml", "UiButton.qml", "UiToolTip.qml", "UiIcon.qml", "UiComboBox.qml"})
            QVERIFY(QFile::copy(source + "/qml/components/" + name, components + '/' + name));
        QVERIFY(QFile::copy(source + "/qml/Theme.qml", directory.path() + "/qml/Theme.qml"));
        QFile module(directory.path() + "/qml/qmldir"); QVERIFY(module.open(QIODevice::WriteOnly));
        module.write("singleton Theme 1.0 Theme.qml\n");
    }
    void translationCatalogsHaveTextAndMatchingPlaceholders()
    {
        const QRegularExpression placeholders("%[0-9]+|%n");
        auto tokens = [&placeholders](const QString &text) {
            QStringList result; auto match = placeholders.globalMatch(text);
            while (match.hasNext()) result.append(match.next().captured());
            result.sort(); return result;
        };
        for (const auto &language : {"en", "zh_CN"}) {
            QFile file(QString::fromUtf8(UI_SOURCE_DIR) + "/translations/lianwall-gui_" + language + ".ts");
            QVERIFY(file.open(QIODevice::ReadOnly)); QXmlStreamReader xml(&file);
            QString source; int count = 0;
            while (!xml.atEnd()) {
                xml.readNext();
                if (!xml.isStartElement()) continue;
                if (xml.name() == QLatin1String("source")) source = xml.readElementText();
                else if (xml.name() == QLatin1String("translation")) {
                    QVERIFY(xml.attributes().value("type") != QLatin1String("unfinished"));
                    const auto translated = xml.readElementText();
                    QVERIFY2(!translated.isEmpty(), qPrintable(source));
                    QCOMPARE(tokens(source), tokens(translated)); ++count;
                }
            }
            QVERIFY2(!xml.hasError(), qPrintable(xml.errorString())); QVERIFY(count > 0);
        }
    }

    void numberEnterEscapeValidationAndFractionalValues()
    {
        SettingsFixture fixture; fixture.values.insert("algorithm.temperature",0.28);
        QQmlEngine engine; engine.rootContext()->setContextProperty("ConfigManager",&fixture); engine.rootContext()->setContextProperty("DaemonState",&fixture);
        QQmlComponent component(&engine,QUrl::fromLocalFile(components + "/NumberSetting.qml"));
        QScopedPointer<QObject> object(component.create()); QVERIFY2(object, qPrintable(component.errorString()));
        object->setProperty("settingKey","algorithm.temperature"); object->setProperty("decimals",2); object->setProperty("minValue",0.01); object->setProperty("maxValue",5);
        auto *root=qobject_cast<QQuickItem*>(object.data()); QVERIFY(root);root->setWidth(700);root->setHeight(root->implicitHeight());
        QQuickWindow window; window.resize(740,300);root->setParentItem(window.contentItem());window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *field=itemNamed(root,"field_algorithm.temperature");QVERIFY(field);
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);typeAscii(&window,"0.75");QTest::keyClick(&window,Qt::Key_Return);
        QCOMPARE(fixture.writes.size(),1);QCOMPARE(fixture.writes.last().second.toDouble(),0.75);QVERIFY(!field->hasActiveFocus());
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);typeAscii(&window,"2.12");QTest::keyClick(&window,Qt::Key_Escape);
        QCOMPARE(field->property("text").toString(),QString("0.75"));QCOMPARE(fixture.writes.size(),1);QVERIFY(!field->hasActiveFocus());
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);QTest::keyClick(&window,Qt::Key_Backspace);QTest::keyClick(&window,Qt::Key_Return);
        QCOMPARE(fixture.writes.size(),1);QVERIFY(!object->property("errorText").toString().isEmpty());QVERIFY(field->hasActiveFocus());
        typeAscii(&window,"0.755");QTest::keyClick(&window,Qt::Key_Return);QCOMPARE(fixture.writes.size(),2);QCOMPARE(fixture.writes.last().second.toDouble(),0.76);QCOMPARE(field->property("text").toString(),QString("0.76"));
        root->setParentItem(nullptr);
    }
    void themedComboBoxSupportsKeyboardSelection()
    {
        QQmlEngine engine;QQmlComponent component(&engine,QUrl::fromLocalFile(components + "/UiComboBox.qml"));
        QScopedPointer<QObject> object(component.create());QVERIFY2(object,qPrintable(component.errorString()));
        object->setProperty("model",QStringList{"One","Two","Three"});
        auto *root=qobject_cast<QQuickItem*>(object.data());QVERIFY(root);root->setWidth(220);root->setHeight(34);
        QQuickWindow window;window.resize(740,300);root->setParentItem(window.contentItem());window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        root->forceActiveFocus();QTest::keyClick(&window,Qt::Key_Down);QCOMPARE(object->property("currentIndex").toInt(),1);QCOMPARE(object->property("currentText").toString(),QString("Two"));
        QTest::keyClick(&window,Qt::Key_Space);
        QQuickItem *option = nullptr;
        QTRY_VERIFY((option = itemNamed(window.contentItem(), "comboOption_2")) != nullptr);
        QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,option->mapToScene(QPointF(option->width()/2,option->height()/2)).toPoint());
        QTRY_COMPARE(object->property("currentIndex").toInt(),2);
        QTest::keyClick(&window,Qt::Key_Space);QTest::keyClick(&window,Qt::Key_Escape);
        QCOMPARE(object->property("currentIndex").toInt(),2);
        root->setParentItem(nullptr);
    }

    void textShowsCanonicalSnapshotAndEscapeCancels()
    {
        SettingsFixture fixture; fixture.values.insert("paths.image_dir", "/old/images"); fixture.canonicalOverrides.insert("paths.image_dir", "/canonical/images");
        QQmlEngine engine;engine.rootContext()->setContextProperty("ConfigManager",&fixture);engine.rootContext()->setContextProperty("DaemonState",&fixture);
        QQmlComponent component(&engine,QUrl::fromLocalFile(components + "/TextSetting.qml"));
        QScopedPointer<QObject> object(component.create());QVERIFY2(object,qPrintable(component.errorString()));object->setProperty("settingKey","paths.image_dir");
        auto *root=qobject_cast<QQuickItem*>(object.data());QVERIFY(root);root->setWidth(700);root->setHeight(root->implicitHeight());
        QQuickWindow window;window.resize(740,300);root->setParentItem(window.contentItem());window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *field=itemNamed(root,"field_paths.image_dir");QVERIFY(field);
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);typeAscii(&window,"~/images");QTest::keyClick(&window,Qt::Key_Return);
        QCOMPARE(fixture.writes.size(),1);QCOMPARE(fixture.writes.last().second.toString(),QString("~/images"));QCOMPARE(field->property("text").toString(),QString("/canonical/images"));QVERIFY(!field->hasActiveFocus());
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);typeAscii(&window,"/discard");QTest::keyClick(&window,Qt::Key_Escape);
        QCOMPARE(field->property("text").toString(),QString("/canonical/images"));QCOMPARE(fixture.writes.size(),1);
        root->setParentItem(nullptr);
    }

    void argumentsPreserveSpacesQuotesEmptyEntriesAndFailedDraft()
    {
        SettingsFixture fixture;fixture.values.insert("video_engine.mpv_args",QStringList{"--flag","--title=hello world","",QString::fromUtf8("空 格与引号\"")});
        QQmlEngine engine;engine.rootContext()->setContextProperty("ConfigManager",&fixture);engine.rootContext()->setContextProperty("DaemonState",&fixture);
        QQmlComponent component(&engine,QUrl::fromLocalFile(components + "/ArgumentsSetting.qml"));
        QScopedPointer<QObject> object(component.create());QVERIFY2(object,qPrintable(component.errorString()));object->setProperty("settingKey","video_engine.mpv_args");
        auto *root=qobject_cast<QQuickItem*>(object.data());QVERIFY(root);root->setWidth(700);root->setHeight(320);
        QQuickWindow window;window.resize(740,400);root->setParentItem(window.contentItem());window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *field=itemNamed(root,"argument_video_engine.mpv_args_1");QVERIFY(field);
        const QString literal="--title=two words \"quoted\"";
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);typeAscii(&window,literal);
        fixture.failNext=true;QTest::keyClick(&window,Qt::Key_Return);QCOMPARE(fixture.writes.size(),1);QVERIFY(object->property("dirty").toBool());QCOMPARE(object->property("errorText").toString(),QString("save failed"));
        const auto args=QJsonValue::fromVariant(fixture.writes.last().second).toArray();QCOMPARE(args.size(),4);QCOMPARE(args[1].toString(),literal);QCOMPARE(args[2].toString(),QString());QCOMPARE(args[3].toString(),QString::fromUtf8("空 格与引号\""));
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_Escape);QVERIFY(!object->property("dirty").toBool());QCOMPARE(field->property("text").toString(),QString("--title=hello world"));
        field->forceActiveFocus();QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);typeAscii(&window,literal);QTest::keyClick(&window,Qt::Key_Return);
        QCOMPARE(fixture.writes.size(),2);QVERIFY(!object->property("dirty").toBool());QVERIFY(!field->hasActiveFocus());
        root->setParentItem(nullptr);
    }
};
int main(int argc,char **argv)
{
    QTemporaryDir directory;
    qputenv("XDG_CONFIG_HOME",directory.path().toUtf8());qputenv("XDG_CACHE_HOME",directory.path().toUtf8());
    QApplication application(argc,argv);TestSettings test;return QTest::qExec(&test,argc,argv);
}
#include "TestSettings.moc"
