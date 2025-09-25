#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ElaWindow.h"

#include <QMainWindow>
#include <toml++/toml.hpp>
class HomePage;
class AboutDialog;
class DefaultPromptPage;
class CommonGptDictPage;
class CommonNormalDictPage;
class SettingPage;
class ProjectSettingsPage;
class ElaContentDialog;
class UpdateChecker;

class MainWindow : public ElaWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public Q_SLOTS:
    void checkUpdate();

protected:
    virtual void mouseReleaseEvent(QMouseEvent* event);

private Q_SLOTS:
    void _onNewProjectTriggered();
    void _onOpenProjectTriggered();
    void _onRemoveProjectTriggered();
    void _onDeleteProjectTriggered();
    void _onSaveProjectTriggered();
    void _onFinishTranslating(QString nodeKey);
    void _onCloseWindowClicked(bool restart);

private:

    void initWindow();
    void initEdgeLayout();
    void initContent();

    HomePage* _homePage{nullptr};
    AboutDialog* _aboutDialog{nullptr};
    DefaultPromptPage* _defaultPromptPage{nullptr};
    CommonNormalDictPage* _commonPreDictPage{nullptr};
    CommonGptDictPage* _commonGptDictPage{nullptr};
    CommonNormalDictPage* _commonPostDictPage{nullptr};
    SettingPage* _settingPage{nullptr};

    QString _commonDictExpanderKey;
    QString _projectExpanderKey;

    QString _aboutKey;
    QString _transIllustrationKey;
    QString _settingKey;

    QList<QSharedPointer<ProjectSettingsPage>> _projectPages;

    ElaContentDialog* _closeDialog{nullptr};
    UpdateChecker* _updateChecker{nullptr};

    toml::table _globalConfig;
};
#endif // MAINWINDOW_H
