#ifndef WEBSINGLEPAGEVIEW_H
#define WEBSINGLEPAGEVIEW_H

#include <QObject>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QEvent>
#include <QFileDialog>

//#include "urfidwrapper.h"
#include <QApplication>

QT_BEGIN_NAMESPACE
class QWebEngineView;
QT_END_NAMESPACE


class WebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    WebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

protected:
    bool certificateError(const QWebEngineCertificateError &error) override;
//    QWebEnginePage *createWindow(WebWindowType) override;
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                        int lineNumber, const QString &sourceID);
    QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                        const QStringList &acceptedMimeTypes);

private slots:
    void doAuthRequired(const QUrl &requestUrl, QAuthenticator *auth);
    void doProxyAuthRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost);
};


class WebPageView : public QWebEngineView
{
    Q_OBJECT
public:
    WebPageView(QWidget *parent = nullptr);
    void setPage(WebPage *page);
    int linkChildEvent();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

    void childEvent(QChildEvent *evt);
    bool eventFilter(QObject *obj, QEvent *_event);

signals:
    void loadProgressStatus(int progress);
    void naviActionChanged(QWebEnginePage::WebAction, bool enabled);
    void linkHovered(QUrl url);
    void onDownloading(QString prompt);

private:
    int m_loadProgress = 0;
    QList<QWebEngineDownloadItem *> m_downloads;
//    URfidWrapper m_urfidWrapper;
    bool g_ImeEnable = false;

private Q_SLOTS:
    void startDownloads(QWebEngineDownloadItem *download);

public slots:
    void setImeEnable(bool enable)  {
        g_ImeEnable = enable;
    }

};

#endif // WEBSINGLEPAGEVIEW_H
