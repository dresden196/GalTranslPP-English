#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <ElaDialog.h>

class AboutDialog : public ElaDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog();
    void setDownloadButtonEnabled(bool enabled);

Q_SIGNALS:
    void checkUpdateSignal();
    void downloadUpdateSignal();

private:
    QAction* _downloadUpdateAction;
};

#endif // ABOUTDIALOG_H
