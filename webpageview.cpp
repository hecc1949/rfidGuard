#include "webpageview.h"
//#include "ui_passworddlg.h"
#include <QMenu>
#include <QFileDialog>
#include <QTreeView>
#include <QMessageBox>

#include <QContextMenuEvent>
#include <QAuthenticator>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineCookieStore>
#include <QWebChannel>


//--------------------------------------------------------------------------------------------------
WebPage::WebPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
    connect(this, &QWebEnginePage::authenticationRequired, this, &WebPage::doAuthRequired);
    connect(this, &QWebEnginePage::proxyAuthenticationRequired, this, &WebPage::doProxyAuthRequired);
}

bool WebPage::certificateError(const QWebEngineCertificateError &error)         //virtual
{
    QWidget *mainWindow = view()->window();
    QMessageBox::critical(mainWindow, tr("认证错误."), tr("网页错误:")+error.errorDescription());
    return false;
}

bool WebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool /* isMainFrame*/)  //virtual
{
    Q_UNUSED(type);
    Q_UNUSED(url);
//#    if (url.scheme() != QString("file"))
//        return(false);

    return(true);
}

QStringList WebPage::chooseFiles(FileSelectionMode mode, const QStringList &oldFiles,
                    const QStringList &acceptedMimeTypes)
{
    Q_UNUSED(oldFiles);

    QString dir = QCoreApplication::applicationDirPath();
    QStringList filter;
    if (acceptedMimeTypes.contains("text/plain"))
    {
        filter<<"text files(*.txt *.csv *.json)";
        if (QDir(dir+"/csv/").exists())
            dir = dir + "/csv";
        else if (QDir(dir+"/txt/").exists())
            dir = dir + "/txt";
    }
    else if (acceptedMimeTypes.contains("image/*"))
    {
        filter<<"image files(*.jpg *.png *.bmp)";
    }

    QFileDialog* selfile = new QFileDialog();
    selfile->setNameFilters(filter);
    selfile->setDirectory(dir);

    selfile->setAcceptMode(QFileDialog::AcceptOpen);
    selfile->setWindowTitle("选择文件");      //无效
    selfile->setLabelText(QFileDialog::LookIn,"路径");
    selfile->setLabelText(QFileDialog::FileName,"文件名");
    selfile->setLabelText(QFileDialog::FileType,"文件类型");
    selfile->setLabelText(QFileDialog::Accept, "选择");
    selfile->setLabelText(QFileDialog::Reject, "取消");
    selfile->setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly);   //不用系统Gtk Dialog
    selfile->setViewMode( QFileDialog::Detail);

    if (mode == QWebEnginePage::FileSelectOpenMultiple)
    {
        //实现多文件选择
        selfile->setFileMode(QFileDialog::ExistingFiles);    //按住Ctrl可以多选
        QTreeView *pTreeView = selfile->findChild<QTreeView*>("treeView");
        if (pTreeView)
        {
            pTreeView->setSelectionMode(QAbstractItemView::MultiSelection);
        }
    }
    else
    {
        selfile->setFileMode(QFileDialog::ExistingFile);
    }
    selfile->setFixedSize(720, 720);

    QStringList filenames;
    if (selfile->exec()==QDialog::Accepted)
        filenames = selfile->selectedFiles();
    delete selfile;
    return(filenames);
}

//将js输出的debug信息导出到qDebug终端输出
void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                              int lineNumber, const QString &sourceID)      //virtual
{
    if (level == QWebEnginePage::InfoMessageLevel)      //console.log()输出，default关闭了
        qDebug()<<"js log:"<<message;
    else if (level ==QWebEnginePage::WarningMessageLevel)
        qDebug()<<"js warning:"<<message;
    else if (level == QWebEnginePage::ErrorMessageLevel)
        qDebug()<<"js error:"<<message<<" @"<<sourceID<<" : line= "<<lineNumber;
}

void WebPage::doAuthRequired(const QUrl &requestUrl, QAuthenticator *auth)      //slot
{
    Q_UNUSED(requestUrl);
    Q_UNUSED(auth);
}

void WebPage::doProxyAuthRequired(const QUrl &, QAuthenticator *auth, const QString &proxyHost)     //slot
{
    Q_UNUSED(auth);
    qDebug()<<"-------- proxy authentication Requred";
    Q_UNUSED(proxyHost);
}

//--------------------------------------------------------------------------------------------------

WebPageView::WebPageView(QWidget *parent)
    : QWebEngineView(parent), m_downloads()     //, m_urfidWrapper(parent)
{
    //这里只是对原有signal作简化封装，意义不大
    connect(this, &QWebEngineView::loadStarted, [this]() {
        m_loadProgress = 0;
        emit loadProgressStatus(m_loadProgress);
    });
    connect(this, &QWebEngineView::loadProgress, [this](int progress) {
        m_loadProgress = progress;
        emit loadProgressStatus(m_loadProgress);
    });
    connect(this, &QWebEngineView::loadFinished, [this](bool success) {
//        m_loadProgress = success ? 100 : -1;
        m_loadProgress = success ? 101 : -1;        //loadProgress中即使没有网页也会出现0,10,70,100这些值，100不表示load成功，用101表示
        emit loadProgressStatus(m_loadProgress);
    });
    connect(this, &QWebEngineView::renderProcessTerminated,
            [this](QWebEnginePage::RenderProcessTerminationStatus termStatus, int statusCode) {
        Q_UNUSED(termStatus);
        Q_UNUSED(statusCode);
        m_loadProgress = -1;
        emit loadProgressStatus(m_loadProgress);
    });

    //downloads
    connect(page()->profile(), &QWebEngineProfile::downloadRequested, this, &WebPageView::startDownloads);
    page()->profile()->clearHttpCache();

    //Settings
    QWebEngineSettings *defSettings = QWebEngineSettings::globalSettings();     //page()->profile()->settings();
    defSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    defSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    defSettings->setAttribute(QWebEngineSettings::PluginsEnabled, true);        //插件，包括flash
    defSettings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    defSettings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, false);
    defSettings->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);

//#    page()->profile()->cookieStore()->deleteAllCookies();
    //
/*    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("urfidwrapper"), &m_urfidWrapper);
    page()->setWebChannel(channel);

    connect(&m_urfidWrapper, &URfidWrapper::setImeEnble, [this](bool enable)  {
        g_ImeEnable = enable;
    });
    */

}

//--- 两个virtual函数的重载 ---

//重载createWindow()使网页在当前window打开而不是tabWidget打开,必须！
QWebEngineView *WebPageView::createWindow(QWebEnginePage::WebWindowType type)     //virtual
{
    Q_UNUSED(type);
    return this;
}

//右键菜单调整
void WebPageView::contextMenuEvent(QContextMenuEvent *event)      //virtual
{
    QMenu *menu = page()->createStandardContextMenu();
    const QList<QAction*> actions = menu->actions();
    auto it = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::OpenLinkInThisWindow));    //"follow link"
    if (it != actions.cend())    {
        (*it)->setText(tr("打开链接"));
    }

    //删除不必要的
    it = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::CopyLinkToClipboard));  //"Copy link URL"
    if (it != actions.cend())    {
        menu->removeAction(*it);
    }
    it = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::DownloadLinkToDisk));  //"save link"
    if (it != actions.cend())    {
        menu->removeAction(*it);
    }
    it = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::ViewSource));  //"ViewSource"
    if (it != actions.cend())    {
        menu->removeAction(*it);
    }

    //改名
    menu->addSeparator();
    QAction* action = page()->action(QWebEnginePage::SavePage);
    action->setText("保存网页");    //QWebEngineDownloadItem::SavePage
    menu->addAction(action);

    menu->popup(event->globalPos());
}


void WebPageView::setPage(WebPage *page)
{
    QWebEngineView::setPage(page);

    //挂接WebChannel对象
/*    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("urfidWrapper"), &m_urfidWrapper);
    page->setWebChannel(channel);
*/

    //链接导航动作
    QAction *action = page->action(QWebEnginePage::Forward);
    connect(action, &QAction::changed, [this, action]    {
        emit naviActionChanged(QWebEnginePage::Forward, action->isEnabled());
    });
    action = page->action(QWebEnginePage::Back);
    connect(action, &QAction::changed, [this, action]    {
        emit naviActionChanged(QWebEnginePage::Back, action->isEnabled());
    });
    action = page->action(QWebEnginePage::Reload);
    connect(action, &QAction::changed, [this, action]    {
        emit naviActionChanged(QWebEnginePage::Reload, action->isEnabled());
    });
    action = page->action(QWebEnginePage::Stop);
    connect(action, &QAction::changed, [this, action]    {
        emit naviActionChanged(QWebEnginePage::Stop, action->isEnabled());
    });

    connect(page, &QWebEnginePage::linkHovered, [this](const QString &url) {
        emit linkHovered(url);
    });
}


void WebPageView::startDownloads(QWebEngineDownloadItem *download)    //slot
{
    Q_ASSERT(download && download->state() == QWebEngineDownloadItem::DownloadRequested);
    if (!m_downloads.isEmpty())
    {
        if (m_downloads.last()->state() == QWebEngineDownloadItem::DownloadInProgress)
            return;
    }

    //设定保存文件的路径和文件类型
    QString path;
    if (download->type() == QWebEngineDownloadItem::SavePage)
    {
        //SavePage隐含用的是mhtml， meta元素没必要，改成简单html格式
        QFileInfo fi(download->path());
        QString defname = fi.absolutePath() +"/" + fi.completeBaseName() + ".html";
        path = QFileDialog::getSaveFileName(this, tr("保存为"), defname, tr("HTML(*.html)"));

        download->setSavePageFormat(QWebEngineDownloadItem::SingleHtmlSaveFormat);
    }
    else
    {
        path = QFileDialog::getSaveFileName(this, tr("保存为"), download->path()); //DownloadAttribute
    }
    if (path.isEmpty())
        return;
//    qDebug()<<"---- request download:"<<download->type();

    m_downloads.append(download);
    connect(download, &QWebEngineDownloadItem::downloadProgress,[this](qint64 bytesReceived, qint64 bytesTotal)    {
        QString prompt = QString("已下载:%1/全部:%2Bytes").arg(bytesReceived).arg(bytesTotal);
        emit onDownloading(prompt);
    });

//#    connect(download, &QWebEngineDownloadItem::stateChanged,[this, download]()   {
    connect(download, &QWebEngineDownloadItem::finished,[this, download]()   {
        auto it = std::find(m_downloads.cbegin(), m_downloads.cend(), download);
        if (it != m_downloads.cend())
        {
            m_downloads.removeOne(*it);
        }
        QString prompt = "下载已完成" + download->path()+QString("   待下载：%1").arg(m_downloads.count());
        emit onDownloading(prompt);
    });

    qDebug()<<"download to:"<<path;
    download->setPath(path);
    download->accept();
}

//------- 以下2个函数为IME接口相关:   ----------
//  控制软键盘IME的开/关要应答发给widget的QInputMethodQueryEvent，
//  web页面上的widget为从child-widget，要遍历child widget给它们都加上eventFilter，在eventFilter中应答Qt::ImEnabled query
//
void WebPageView::childEvent(QChildEvent *evt)        //virtual
{
    if (evt->polished())
    {
        if (evt->child() && evt->child()->isWidgetType())   {
            evt->child()->installEventFilter(this);
        }
    }
}
//
bool WebPageView::eventFilter(QObject *obj, QEvent *_event)      //virtual
{
    if (_event->type()==QEvent::InputMethodQuery)
    {
        QInputMethodQueryEvent *evt = static_cast<QInputMethodQueryEvent*>(_event);
        if (evt->queries() == Qt::ImEnabled)
        {
            evt->setValue(Qt::ImEnabled, g_ImeEnable);    //在此根据js发来的信号，设置为true/false可以开/关IME
            return(true);       //必须的
        }
    }
    return QWebEngineView::eventFilter(obj, _event);
}

