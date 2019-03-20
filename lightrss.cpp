/********************************************************************

Name: lightrss
Homepage: http://github.com/deussomniat/lightrss
Author: deussomniat
Description: A simple qt based rss feed reader

Copyright (C) 2017 deussomniat

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#include "lightrss.h"

Template::Template(QString tpl)
{
    tplName = tpl;
}

lightrss::lightrss(QWidget *parent)
    : QMainWindow(parent)
{
    userAgent = "lightrss/0.3";

    acceptedXmlTypes << "application/rss+xml" << "application/x-rss+xml"
                     << "application/rdf+xml" << "application/x-rdf+xml"
                     << "application/atom+xml" << "application/x-atom+xml"
                     << "application/xml" << "application/x-xml"
                     << "text/xml";

    acceptedImgTypes << "image/jpeg" << "image/png" << "image/gif";

    acceptedXmlExtensions << "xml" << "rss";
    acceptedImgExtensions << "jpg" << "jpeg" << "gif" << "png";

    // must match order of MapRoles enum
    mapList << "feed_title" << "feed_image" << "item_title"
            << "item_date" << "item_webpage" << "item_download"
            << "item_size" << "item_duration" << "item_description";

    sep = QDir::separator();
    tempPath = QDir::tempPath();
    homePath = tr("%1%2%3").arg(QDir::homePath()).arg(sep).arg(".lightrss");
    catalogPath = tr("%1%2%3").arg(homePath).arg(sep).arg("catalog.xml");
    feedsPath = tr("%1%2%3").arg(homePath).arg(sep).arg("feeds");
    imagesPath = tr("%1%2%3").arg(homePath).arg(sep).arg("images");
    templatesPath = tr("%1%2%3").arg(homePath).arg(sep).arg("templates");

    // make sure all lightrss folders exist
    QDir dirTest;
    dirTest.setPath(homePath);
    if (!dirTest.exists()) dirTest.mkdir(homePath);
    dirTest.setPath(feedsPath);
    if (!dirTest.exists()) dirTest.mkdir(feedsPath);
    dirTest.setPath(imagesPath);
    if (!dirTest.exists()) dirTest.mkdir(imagesPath);
    dirTest.setPath(templatesPath);
    if (!dirTest.exists()) dirTest.mkdir(templatesPath);

    // make sure catalog xml file exists and is
    // readable and writable
    QFile fileTest;
    fileTest.setFileName(catalogPath);
    if (!fileTest.exists()) QFile::copy(":/templates/catalog.xml", catalogPath);
    if (!fileTest.isWritable()) QFile::setPermissions(catalogPath, QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther | QFile::WriteOwner);

    namgr.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    createWidgets();
    // load default template. loadFeeds will also
    // load all templates and we want the default
    // template to always be first in the QList.
    loadTemplate(":/templates/default.xml");
    loadFeeds();
    connectEvents();

    setMinimumWidth(1060);
    setMinimumHeight(485);
    setWindowTitle("lightrss");
    setWindowIcon(QIcon(":/images/rss48.png"));

    // process any urls that were passed to the app
    QStringList urls = qApp->arguments();
    for (int i = 1; i < urls.length(); i++) {
        qDebug() << tr("processing url %1 of %2").arg(i).arg(urls.length() - 1);
        urlTextBox->setText(urls[i]);
        startDownload(urls[i]);
    }
}

lightrss::~lightrss()
{

}

void lightrss::createWidgets()
{
    addImgWin = new AddImage;
    addTplWin = new AddTemplate(templatesPath);

    backAction = new QAction(QIcon(":/images/back48.png"), "");
    backAction->setStatusTip(tr("Back"));
    backAction->setDisabled(1);
    addFeedAction = new QAction(QIcon(":/images/add48.png"), "");
    addFeedAction->setStatusTip(tr("Add new feed"));
    updateAllAction = new QAction(QIcon(":/images/update48.png"), "");
    updateAllAction->setStatusTip(tr("Update all feeds"));
    saveAction = new QAction(QIcon(":/images/save48.png"), "");
    saveAction->setStatusTip(tr("Save catalog"));

    copyFeedAction = new QAction(QIcon(":/images/copy_feed48.png"), tr("Copy feed url"));
    refreshFeedAction = new QAction(QIcon(":/images/feed48.png"), tr("Refresh feed"));
    refreshImageAction = new QAction(QIcon(":/images/image48.png"), tr("Refresh image"));
    addImageAction = new QAction(QIcon(":/images/addimage48.png"), tr("Add image"));
    addTemplateAction = new QAction(QIcon(":/images/addtemplate48.png"), tr("Add template"));
    deleteFeedAction = new QAction(QIcon(":/images/delete48.png"), tr("Delete"));

    copyItemAction = new QAction(QIcon(":/images/copy_item48.png"), tr("Copy item url"));
    copyEncAction = new QAction(QIcon(":/images/copy_enclosure48.png"), tr("Copy enclosure url"));

    urlTextBox = new QLineEdit;
    urlTextBox->setStatusTip("Enter feed url");

    QToolBar *toolbar = new QToolBar;
    toolbar->addAction(backAction);
    toolbar->addWidget(urlTextBox);
    toolbar->addAction(addFeedAction);
    toolbar->addAction(updateAllAction);
    toolbar->addAction(saveAction);
    addToolBar(toolbar);

    statusbar = new QStatusBar;
    setStatusBar(statusbar);

    feedContextMenu = new QMenu;
    feedContextMenu->addAction(copyFeedAction);
    feedContextMenu->addAction(refreshFeedAction);
    feedContextMenu->addAction(refreshImageAction);
    feedContextMenu->addAction(addImageAction);
    feedContextMenu->addAction(addTemplateAction);
    feedContextMenu->addSeparator();
    feedContextMenu->addAction(deleteFeedAction);

    itemContextMenu = new QMenu;
    itemContextMenu->addAction(copyItemAction);
    itemContextMenu->addAction(copyEncAction);

    feedTable = new TableWidget;
    feedTable->setShowGrid(0);
    feedTable->setColumnCount(5);
    feedTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    feedTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    feedTable->verticalHeader()->setVisible(0);
    feedTable->horizontalHeader()->setVisible(0);
    feedTable->horizontalHeader()->setStretchLastSection(0);
    feedTable->horizontalHeader()->setDefaultSectionSize(206);
    feedTable->verticalHeader()->setDefaultSectionSize(206);
    feedTable->setContextMenuPolicy(Qt::CustomContextMenu);
    feedTable->setSelectionMode(QAbstractItemView::SingleSelection);
    feedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    feedTable->setDragEnabled(1);
    feedTable->setAcceptDrops(1);
    feedTable->setDropIndicatorShown(1);
    feedTable->setDragDropMode(QAbstractItemView::InternalMove);
    feedTable->setMouseTracking(1);
    feedTable->viewport()->setAcceptDrops(1);
    feedTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    feedTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    feedTable->setFixedWidth(1046);

    titleLabel = new QLabel;
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: 700; }");
    encLabel = new QLabel;
    encLabel->setOpenExternalLinks(1);
    linkLabel = new QLabel;
    linkLabel->setOpenExternalLinks(1);
    sizeLabel = new QLabel;
    durationLabel = new QLabel;

    descBrowser = new QTextBrowser;
    descBrowser->setOpenLinks(1);
    descBrowser->setOpenExternalLinks(1);
    descBrowser->setStyleSheet("QTextBrowser { background:#ffffff; }");

    itemTable = new QTableWidget;
    itemTable->setShowGrid(1);
    itemTable->setColumnCount(2);
    itemTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    itemTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    itemTable->verticalHeader()->setVisible(0);
    itemTable->horizontalHeader()->setVisible(1);
    itemTable->horizontalHeader()->setStretchLastSection(1);
    itemTable->setContextMenuPolicy(Qt::CustomContextMenu);
    itemTable->setHorizontalHeaderLabels(QStringList() << "Title" << "Date");
    itemTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    itemTable->setSelectionMode(QAbstractItemView::SingleSelection);
    itemTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QHBoxLayout *linksLayout = new QHBoxLayout;
    linksLayout->addWidget(linkLabel, 1, Qt::AlignCenter);
    linksLayout->addWidget(encLabel, 1, Qt::AlignCenter);
    linksLayout->addWidget(sizeLabel, 1, Qt::AlignCenter);
    linksLayout->addWidget(durationLabel, 1, Qt::AlignCenter);

    QVBoxLayout *itemLayout = new QVBoxLayout;
    itemLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    itemLayout->addWidget(itemTable, 1);
    itemLayout->addLayout(linksLayout);
    itemLayout->addWidget(descBrowser, 1);

    QWidget *itemWidget = new QWidget;
    itemWidget->setLayout(itemLayout);

    QVBoxLayout *feedLayout = new QVBoxLayout;
    feedLayout->addWidget(feedTable, 1, Qt::AlignHCenter | Qt::AlignTop);

    QWidget *feedWidget = new QWidget;
    feedWidget->setLayout(feedLayout);

    tableStack = new QStackedWidget;
    tableStack->addWidget(feedWidget);
    tableStack->addWidget(itemWidget);

    setCentralWidget(tableStack);
}

void lightrss::loadTemplate(QString tpl)
{
    for (int i = 0; i < templates.length(); i++) {
        // no need to process template if it already exists
        if (templates[i].tplName == tpl) return;
    }

    // the default template has the path :/templates/default.xml
    QString path = (tpl.left(2) == ":/") ? tpl : tr("%1%2%3").arg(templatesPath).arg(sep).arg(tpl);
    tpl = tpl.remove(QRegExp("^.*/"));

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return;
    }

    QDomDocument tdoc("template");
    if (!tdoc.setContent(&file)) {
        file.close();
        return;
    }

    file.close();

    QDomNodeList maps = tdoc.elementsByTagName("map");
    if (maps.length() < 1) return;

    // set the name of the new template
    Template tobj(tpl);

    QRegExp re("(@*[^\\.@]+)");
    QStringList reList;
    int pos = 0;

    QDomNode mapNode;
    QString srcStr, dstStr;
    for (int i = 0; i < maps.length(); i++) {
        mapNode = maps.at(i);
        if (mapNode.isNull() || !mapNode.isElement()) continue;
        srcStr = mapNode.toElement().attribute("src");
        dstStr = mapNode.toElement().attribute("dst");
        if (srcStr.isEmpty() || dstStr.isEmpty()) continue;

        pos = 0;
        reList.clear();
        while ((pos = re.indexIn(srcStr, pos)) != -1) {
              reList << re.cap(1);
              pos += re.matchedLength();
        }

        switch (mapList.indexOf(dstStr)) {
            case FeedTitle:
                tobj.feedTitle << reList;
                break;

            case FeedImage:
                tobj.feedImage << reList;
                break;

            case ItemTitle:
                tobj.itemTitle << reList;
                break;

            case ItemDate:
                tobj.itemDate << reList;
                break;

            case ItemWebpage:
                tobj.itemWebpage << reList;
                break;

            case ItemDownload:
                tobj.itemDownload << reList;
                break;

            case ItemSize:
                tobj.itemSize << reList;
                break;

            case ItemDuration:
                tobj.itemDuration << reList;
                break;

            case ItemDescription:
                tobj.itemDescription << reList;
                break;

            default:
                continue;
        }
    }
    templates << tobj;
}

void lightrss::loadFeeds()
{
    QFile file(catalogPath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return;
    }

    QDomDocument catalog("catalog");
    if (!catalog.setContent(&file)) {
        file.close();
        return;
    }

    file.close();

    QFileInfo finfo;
    QString imgStr, xmlStr, urlStr, tplStr;
    QDomNode itemNode, imgNode, xmlNode, urlNode, tplNode;
    QDomNodeList items = catalog.elementsByTagName("item");
    for (int i = 0; i < items.length(); i++) {
        itemNode = items.at(i);
        if (itemNode.isNull() || !itemNode.isElement()) continue;

        xmlNode = itemNode.namedItem("xml");
        if (xmlNode.isNull() || !xmlNode.isElement()) continue;

        xmlStr = xmlNode.toElement().text();
        if (xmlStr.isEmpty()) continue;

        finfo.setFile(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlStr));
        if (!finfo.exists()) continue;

        urlNode = itemNode.namedItem("url");
        if (urlNode.isNull() || !urlNode.isElement()) continue;

        urlStr = urlNode.toElement().text();
        if (urlStr.isEmpty()) continue;

        imgStr = "";
        imgNode = itemNode.namedItem("img");
        if (!imgNode.isNull() && imgNode.isElement()) {
            imgStr = imgNode.toElement().text();
            if (!imgStr.isEmpty()) {
                finfo.setFile(tr("%1%2%3").arg(imagesPath).arg(sep).arg(imgStr));
                if (!finfo.exists()) imgStr = "";
            }
        }

        tplStr = "";
        tplNode = itemNode.namedItem("tpl");
        if (!tplNode.isNull() && tplNode.isElement()) {
            tplStr = tplNode.toElement().text();
            if (!tplStr.isEmpty()) {
                finfo.setFile(tr("%1%2%3").arg(templatesPath).arg(sep).arg(tplStr));
                if (!finfo.exists()) {
                    tplStr = "";
                } else {
                    loadTemplate(tplStr);
                }
            }
        }

        catalogList << (QStringList() << xmlStr << urlStr << imgStr << tplStr);
    }

    loadFeedTable();
}

void lightrss::loadFeedTable()
{
    clearFeedTable();

    int row, col, total = 0;
    QTableWidgetItem *thumbnail;

    for (int i = 0; i < catalogList.length(); i++) {
        thumbnail = createFeedTableItem(i);
        if (!thumbnail) continue;

        row = total / 5;
        col = total - (row * 5);
        feedTable->setRowCount(row + 1);
        feedTable->setItem(row, col, thumbnail);
        total++;
    }
}

void lightrss::clearFeedTable()
{
    feedTable->clearSelection();
    while (feedTable->rowCount() > 0) {
        feedTable->removeRow(0);
    }
}

QTableWidgetItem* lightrss::createFeedTableItem(int index)
{
    bool loaded, hasImage;
    QFileInfo finfo;
    QString imgNameStr, xmlNameStr, urlStr;

    // we must have the xml file to create a table item
    xmlNameStr = getFeedXml(index);
    if (xmlNameStr.isEmpty()) return nullptr;

    // make sure local feed exists
    finfo.setFile(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlNameStr));
    if (!finfo.exists()) return nullptr;

    // we must have a feed url to generate a table item
    urlStr = getFeedUrl(index);
    if (urlStr.isEmpty()) return nullptr;

    // make sure local image exists if path is
    // specified in catalog
    imgNameStr = getFeedImg(index);
    hasImage = !imgNameStr.isEmpty();
    if (hasImage) {
        imgNameStr = tr("%1%2%3").arg(imagesPath).arg(sep).arg(imgNameStr);
        finfo.setFile(imgNameStr);
        hasImage = finfo.exists();
    }

    QTableWidgetItem *thumbnail = new QTableWidgetItem;

    if (hasImage) {
        QImage *img = new QImage;
        loaded = img->load(imgNameStr);
        if (loaded) {
            thumbnail->setData(Qt::DecorationRole, QPixmap::fromImage(*img).scaled(200, 200, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        } else {
            qDebug() << tr("%1 was not loaded!").arg(imgNameStr);
        }
    }

    thumbnail->setSizeHint(QSize(206, 206));
    thumbnail->setStatusTip(urlStr);
    thumbnail->setText(xmlNameStr);
    thumbnail->setTextAlignment(Qt::AlignCenter);
    thumbnail->setData(IdRole, index);
    return thumbnail;
}

QString lightrss::getFeedXml(int index)
{
    return (index > -1 && index < catalogList.length()) ? catalogList[index][0] : "";
}

bool lightrss::setFeedXml(int index, QString value)
{
    if (index < 0 || index > catalogList.length() - 1) return 0;
    catalogList[index][0] = value;
    return 1;
}

QString lightrss::getFeedUrl(int index)
{
    return (index > -1 && index < catalogList.length()) ? catalogList[index][1] : "";
}

bool lightrss::setFeedUrl(int index, QString value)
{
    if (index < 0 || index > catalogList.length() - 1) return 0;
    catalogList[index][1] = value;
    return 1;
}

QString lightrss::getFeedImg(int index)
{
    return (index > -1 && index < catalogList.length()) ? catalogList[index][2] : "";
}

bool lightrss::setFeedImg(int index, QString value)
{
    if (index < 0 || index > catalogList.length() - 1) return 0;
    catalogList[index][2] = value;
    return 1;
}

QString lightrss::getFeedTpl(int index)
{
    return (index > -1 && index < catalogList.length() && !catalogList[index][3].isEmpty()) ? catalogList[index][3] : "default.xml";
}

bool lightrss::setFeedTpl(int index, QString value)
{
    if (index < 0 || index > catalogList.length() - 1) return 0;
    catalogList[index][3] = value;
    return 1;
}

void lightrss::connectEvents()
{
    connect(feedTable, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(selectFeed(QTableWidgetItem*)));
    connect(itemTable, SIGNAL(itemSelectionChanged()), this, SLOT(selectItem()));
    connect(feedTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(feedMenuRequested(QPoint)));
    connect(itemTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(itemMenuRequested(QPoint)));
    connect(backAction, SIGNAL(triggered()), this, SLOT(showFeeds()));
    connect(addFeedAction, SIGNAL(triggered()), this, SLOT(addFeed()));
    connect(refreshImageAction, SIGNAL(triggered()), this, SLOT(refreshImage()));
    connect(copyFeedAction, SIGNAL(triggered()), this, SLOT(copyFeedUrl()));
    connect(copyItemAction, SIGNAL(triggered()), this, SLOT(copyItemUrl()));
    connect(copyEncAction, SIGNAL(triggered()), this, SLOT(copyEnclosureUrl()));
    connect(deleteFeedAction, SIGNAL(triggered()), this, SLOT(deleteFeed()));
    connect(refreshFeedAction, SIGNAL(triggered()), this, SLOT(refreshFeed()));
    connect(updateAllAction, SIGNAL(triggered()), this, SLOT(updateAllFeeds()));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveXML()));
    connect(&namgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    connect(urlTextBox, SIGNAL(returnPressed()), this, SLOT(addFeed()));
    connect(addImageAction, SIGNAL(triggered()), addImgWin, SLOT(show()));
    connect(addTemplateAction, SIGNAL(triggered()), this, SLOT(showTplMenu()));
    connect(addTplWin->tplMenu, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(tplSelected(QString)));
    connect(addImgWin->fileBtn, SIGNAL(clicked()), this, SLOT(addImageFile()));
    connect(addImgWin->urlBtn, SIGNAL(clicked()), this, SLOT(addImageUrl()));
}

void lightrss::showTplMenu()
{
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();
    QString tpl = getFeedTpl(index);

    addTplWin->tplLabel->setText(tr("<b>Active:</b> %1").arg(tpl));
    addTplWin->loadMenu();
    addTplWin->show();
}

void lightrss::addImageFile()
{
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select image"),
                                                    imagesPath,
                                                    tr("Images (*.png *.gif *.jpg *.jpeg)"));

    if (fileName.isNull() || fileName.isEmpty()) return;

    // get original file extension
    QFileInfo finfo(fileName);
    QString fext = finfo.suffix().toLower();

    // get new filename for image
    QString feedTitle = getFeedImg(index);
    if (feedTitle.isEmpty()) {
        // we're adding the image for the first time
        feedTitle = getFeedXml(index);
    }

    if (feedTitle.isEmpty()) {
        qDebug() << "The feed title is empty!";
        return;
    }

    // preserve original file extension
    finfo.setFile(feedTitle);
    feedTitle = tr("%1.%2").arg(finfo.baseName()).arg(fext);

    // are the new and old paths the same?
    if (fileName == tr("%1%2%3").arg(imagesPath).arg(sep).arg(feedTitle)) {
        qDebug() << "The old and new image paths are identical!";
        return;
    }

    // does it exist?
    QFile oldFile;
    oldFile.setFileName(tr("%1%2%3").arg(imagesPath).arg(sep).arg(feedTitle));

    if (oldFile.exists() && !oldFile.remove()) {
        qDebug() << "Can't remove existing image!";
        return;
    }

    // rename image
    bool success = QFile::rename(fileName, tr("%1%2%3").arg(imagesPath).arg(sep).arg(feedTitle));
    if (!success) {
        qDebug() << "Can't rename image!";
        return;
    }

    if (!setFeedImg(index, feedTitle)) {
        qDebug() << "Setting image failed!" << tr("Feed %1 - Image %2").arg(index).arg(feedTitle);
        statusbar->showMessage("Setting image failed!", 10000);
    } else {
        statusbar->showMessage("Image successfully added!", 10000);
        updateThumbnail(feedTitle, index);
    }

    addImgWin->hide();
}

void lightrss::addImageUrl()
{
    bool ok;

    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();

    QString url = QInputDialog::getText(this,
                                       tr("Enter image url"),
                                       tr("Enter image url"),
                                       QLineEdit::Normal,
                                       "", &ok);
    if (!ok || url.isEmpty()) return;

    startDownload(url, index);

    addImgWin->hide();
}

void lightrss::tplSelected(QString tplName)
{
    if (addTplWin->tplMenu->currentIndex() < 1 || tplName.isEmpty()) return;

    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();

    if (!setFeedTpl(index, tplName)) {
        qDebug() << "Setting template failed!" << tr("Feed %1 - Template %2").arg(index).arg(tplName);
        statusbar->showMessage("Setting template failed!", 10000);
    } else {
        statusbar->showMessage("Template successfully added!", 10000);
    }

    addTplWin->hide();
}

void lightrss::startDownload(const QString &urlStr, int index)
{
    if (urlStr.isEmpty()) return;

    if (!ncmgr.isOnline() || namgr.networkAccessible() != QNetworkAccessManager::Accessible) {
        qDebug() << "The network is not available.";
        return;
    }

    QUrl url(urlStr);
    if (!url.isValid()) {
        qDebug() << "The url is not valid.";
        return;
    }

    // save url and feed index (if exists) prior to downloading
    trackDownload(urlStr, index);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", userAgent.toLocal8Bit().data());
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, 1);
    request.setMaximumRedirectsAllowed(3);

    QNetworkReply *reply = namgr.get(request);

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#ifndef QT_NO_SSL
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(downloadSslErrors(QList<QSslError>)));
#endif

    currentDownloads.append(reply);
}

void lightrss::trackDownload(QString url, int index)
{
    // the only downloads where we are supplying the
    // feed index will be images.
    if (index > -1) {
        downloadTracker << (QVariantList() << index << url);
        return;
    }

    // if we made it this far we know it's not an image.
    // if we don't find the url in the catalogList then
    // it's either a new feed or an unsupported file.
    for (int i = 0; i < catalogList.length(); i++) {
        if (url == catalogList[i][1]) {
            downloadTracker << (QVariantList() << i << url);
            break;
        }
    }
}

int lightrss::getFeedIndex(QString url)
{
    for (int i = 0; i < downloadTracker.length(); i++) {
        if (url == downloadTracker[i][1].toString()) return downloadTracker[i][0].toInt();
    }
    return -1;
}

QString lightrss::getTempFileName(const QUrl &url)
{
    QString basename = url.fileName();

    if (basename.isEmpty()) basename = "download";

    if (QFile::exists(tr("%1%2%3").arg(tempPath).arg(sep).arg(basename))) {
        // file already exists. create unique file name
        // to avoid overwriting.
        int i = 0;
        basename = basename.append(".");

        while (QFile::exists(tr("%1%2%3").arg(tempPath).arg(sep).arg(basename + QString::number(i)))) ++i;

        basename = basename.append(QString::number(i));
    }

    return basename;
}

QString lightrss::getFeedTitle(const QString &path, int fIndex)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return "";
    }

    QDomDocument doc("rss");
    if (!doc.setContent(&file)) {
        qDebug() << tr("Can't parse xml!") << path;
        file.close();
        return "";
    }

    file.close();

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return "";

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return "";

    QDomNodeList channel = doc.elementsByTagName("channel");
    if (channel.length() < 1) {
        qDebug() << "No channel element found in feed!";
        return "";
    }

    QString titleStr = getXMLValue(channel.at(0), templates[tIndex].feedTitle);
    if (titleStr.isEmpty()) {
        qDebug() << tr("Title element is empty!") << path;
        return "";
    }

    QString xmlFileName = prepFeedTitle(titleStr);

    return xmlFileName;
}

QString lightrss::prepFeedTitle(const QString &title)
{
    QString xmlNameStr = title.toLower();
    xmlNameStr = xmlNameStr.remove(QRegExp("\\|.+$"));
    xmlNameStr = xmlNameStr.replace(QRegExp("[^a-z0-9_]"), "_");
    xmlNameStr = xmlNameStr.replace(QRegExp("_{2,}"), "_");
    xmlNameStr = xmlNameStr.remove(QRegExp("^_|_$"));
    return xmlNameStr;
}

bool lightrss::saveToDisk(const QString &path, QIODevice *data)
{
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << tr("Could not open file for writing!") << path << file.errorString();
        return 0;
    }

    file.write(data->readAll());
    file.close();

    return 1;
}

bool lightrss::isHttpRedirect(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303
           || statusCode == 305 || statusCode == 307 || statusCode == 308;
}

void lightrss::refreshFeed()
{
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();

    QString urlStr = getFeedUrl(index);
    if (urlStr.isEmpty()) return;

    startDownload(urlStr);
}

void lightrss::updateAllFeeds()
{
    QString urlStr;
    for (int i = 0; i < catalogList.length(); i++) {
        urlStr = getFeedUrl(i);
        if (urlStr.isEmpty()) continue;
        startDownload(urlStr);
    }
}

void lightrss::addFeed()
{
    QString urlStr = urlTextBox->text();
    if (urlStr.isEmpty()) return;

    // test for itunes url here and grab id.
    //
    // https://itunes.apple.com/us/podcast/mysterious-universe/id329937558
    // becomes
    // https://itunes.apple.com/lookup?id=329937558

    urlStr = urlStr.toLower();
    QRegExp rx("itunes.apple.com/[^/]+/podcast/");
    if (urlStr.contains(rx)) {
        qDebug() << "itunes url detected!";
        rx.setPattern("/id(\\d+)");
        if (urlStr.contains(rx) && rx.captureCount() > 0 &&
            rx.capturedTexts().length() > 1 && !rx.capturedTexts()[1].isEmpty()) {
            QString id = rx.capturedTexts()[1];
            qDebug() << tr("using itunes id %1").arg(id);
            urlStr = tr("https://itunes.apple.com/lookup?id=%1").arg(id);
        } else {
            qDebug() << tr("cannot extract itunes id from url");
            return;
        }
    }

    startDownload(urlStr);
}

void lightrss::downloadError(QNetworkReply::NetworkError code)
{
    qDebug() << tr("Network error: %1").arg(QString::number(code));
}

void lightrss::downloadSslErrors(const QList<QSslError> &sslErrors)
{
#ifndef QT_NO_SSL
    foreach (const QSslError &error, sslErrors) {
        qDebug() << tr("SSL error: %1").arg(error.errorString());
    }
#else
    Q_UNUSED(sslErrors);
#endif
}

QString lightrss::extractFeedUrl(QNetworkReply *reply)
{
    QString feedUrl = "";

    QByteArray jsonRaw = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) return feedUrl;

    QJsonObject jsonRootObj = jsonDoc.object();
    QJsonArray results = jsonRootObj["results"].toArray();
    if (results.count() < 1) return feedUrl;

    QJsonValue jsonVal = results.at(0);
    QJsonObject feed = jsonVal.toObject();
    return feed["feedUrl"].toString();
}

void lightrss::downloadFinished(QNetworkReply *reply)
{
    bool isError = 0, isImage = 0, isOldFile = 0;
    int prevTotal, index = -1;
    QString mime, filename, feedTitle, feedUrl;
    QUrl url = reply->url();

    if (reply->error()) {
        isError = 1;
        qDebug() << tr("Download of %1 failed: %2").arg(url.toString()).arg(reply->errorString());
    }

    if (!isError) {
        isError = isHttpRedirect(reply);
        if (isError) {
            qDebug() << "Request was redirected.";
        }
    }

    if (!isError) {
        mime = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        isError = mime.isNull() || mime.isEmpty();
        if (isError) {
            qDebug() << "No mime type returned!";
        } else {
            mime = mime.toLower();
            mime = mime.remove(QRegExp(";.*$"));
            qDebug() << url.toString() << mime;
        }
    }

    QFileInfo finfo(url.fileName());
    QString fext = finfo.suffix().toLower();

    if (acceptedXmlTypes.contains(mime) ||
        acceptedXmlExtensions.contains(fext)) {
        isImage = 0;
    } else if (acceptedImgTypes.contains(mime) ||
               acceptedImgExtensions.contains(fext)) {
        isImage = 1;
    } else if (mime == "text/javascript") {
        isError = 1;

        // itunes lookup results are returned as json.
        // extract feedURL and replace url in text box
        feedUrl = extractFeedUrl(reply);

        if (!feedUrl.isEmpty()) {
            qDebug() << "itunes url converted!";
            urlTextBox->setText(feedUrl);
            startDownload(feedUrl);
        }
    } else {
        isError = 1;
        qDebug() << "Unknown mime type detected!";
    }

    if (!isError) {
        filename = getTempFileName(url);
        isError = !saveToDisk(tr("%1%2%3").arg(tempPath).arg(sep).arg(filename), reply);
        if (isError) {
            qDebug() << tr("Cannot save %1 to disk!").arg(filename);
        }
    }

    if (!isError) {
        index = getFeedIndex(url.toString());
        if (index > -1) {
            // feed already exists and we're updating xml or image
            if (!isImage) {
                feedTitle = getFeedXml(index);
            } else {
                feedTitle = getFeedImg(index);
                if (feedTitle.isEmpty()) {
                    // we're downloading the image for the first time
                    feedTitle = getFeedXml(index);
                }
                if (!feedTitle.isEmpty()) {
                    // preserve file extension from url
                    finfo.setFile(feedTitle);
                    feedTitle = tr("%1.%2").arg(finfo.baseName()).arg(fext);
                }
            }
        } else {
            // feed doesn't exist
            if (!isImage) {
                feedTitle = getFeedTitle(tr("%1%2%3").arg(tempPath).arg(sep).arg(filename));
                if (!feedTitle.isEmpty()) {
                    feedTitle = feedTitle.append(".xml");
                }
            } else {
                // why are we downloading an image for a feed that
                // doesn't exist?
                qDebug() << "Image is not associated with any existing feed!" << url.toString();
            }
        }

        isError = feedTitle.isEmpty();
        if (isError) {
            qDebug() << "Cannot find feed title!";
        }
    }

    if (!isError) {
        QFile oldFile;
        if (!isImage) {
            oldFile.setFileName(tr("%1%2%3").arg(feedsPath).arg(sep).arg(feedTitle));
        } else {
            oldFile.setFileName(tr("%1%2%3").arg(imagesPath).arg(sep).arg(feedTitle));
        }
        isOldFile = oldFile.exists();
        if (isOldFile) {
            isError = !oldFile.remove();
            if (isError) {
                qDebug() << "Cannot delete old file!";
            }
        }
    }

    if (!isError) {
        if (!isImage) {
            isError = !QFile::rename(tr("%1%2%3").arg(tempPath).arg(sep).arg(filename),
                                     tr("%1%2%3").arg(feedsPath).arg(sep).arg(feedTitle));
        } else {
            isError = !QFile::rename(tr("%1%2%3").arg(tempPath).arg(sep).arg(filename),
                                     tr("%1%2%3").arg(imagesPath).arg(sep).arg(feedTitle));
        }
        if (isError) {
            qDebug() << "Cannot rename updated file!";
        }
    }

    if (!isError && isImage) {
        updateThumbnail(feedTitle, index);
    }

    if (!isError && !isImage && isOldFile) {
        if (tableStack->currentIndex() == 1) {
            // refresh item table if it's already visible and
            // its feed has just completed an update.
            QTableWidgetItem *ftwi = feedTable->currentItem();
            int fIndex = ftwi->data(IdRole).toInt();
            if (fIndex == index) {
                selectFeed(ftwi);
            }
        }
    }

    if (!isError && !isImage && !isOldFile) {
        prevTotal = catalogList.length();
        index = createCatalogEntry(feedTitle, url.toString());
        isError = (index < 0 || prevTotal == catalogList.length());
        if (isError) {
            qDebug() << "Cannot update catalog with new feed!";
        }
    }

    if (!isError && !isImage && !isOldFile) {
        QTableWidgetItem *thumbnail = createFeedTableItem(index);
        isError = !thumbnail;
        if (isError) {
            qDebug() << "Cannot create new table item!";
        } else {
            feedTable->insertThumbnail(thumbnail);
            refreshImage(index);
        }
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();

    if (currentDownloads.isEmpty()) {
        statusbar->showMessage("All downloads finished!", 10000);
        downloadTracker.clear();
    } else {
        statusbar->showMessage(tr("%1 download%2 remaining").
                               arg(QString::number(currentDownloads.length())).
                               arg((currentDownloads.length() > 1) ? "s" : ""), 10000);
        stopTracking(url.toString());
    }
}

void lightrss::stopTracking(QString url)
{
    for (int i = 0; i < downloadTracker.length(); i++) {
        if (downloadTracker[i][1].toString() == url) {
            downloadTracker.removeAt(i);
            break;
        }
    }
}

void lightrss::updateThumbnail(const QString &feedTitle, int index)
{
    if (index < 0) return;

    if (!setFeedImg(index, feedTitle)) return;

    int twiIndex;
    bool loaded;
    int rows = feedTable->rowCount();
    int cols = feedTable->columnCount();
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            QTableWidgetItem *twi = feedTable->item(r, c);
            if (!twi) continue;
            twiIndex = twi->data(IdRole).toInt();
            if (twiIndex != index) continue;
            QImage *img = new QImage;
            loaded = img->load(tr("%1%2%3").arg(imagesPath).arg(sep).arg(feedTitle));
            if (loaded) {
                twi->setData(Qt::DecorationRole, QPixmap::fromImage(*img).scaled(200, 200, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            } else {
                qDebug() << tr("%1 was not loaded!").arg(feedTitle);
            }
            return;
        }
    }
}

void lightrss::feedMenuRequested(QPoint pos)
{
    // there may be empty cells in the table. we
    // do not want to show context menu for an
    // empty cell. try to retrieve selected item
    // and if null do not show menu.
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    feedContextMenu->popup(feedTable->viewport()->mapToGlobal(pos));
}

void lightrss::itemMenuRequested(QPoint pos)
{
    // it's possible that the user may right click
    // on a pubDate item that's empty. test for
    // empty cell before displaying menu.
    QTableWidgetItem *itwi = itemTable->currentItem();
    if (!itwi) return;

    int iIndex = itwi->data(IdRole).toInt();

    QTableWidgetItem *ftwi = feedTable->currentItem();
    if (!ftwi) return;

    int fIndex = ftwi->data(IdRole).toInt();

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return;

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return;

    QString webStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemWebpage);
    QString downStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemDownload);

    copyItemAction->setDisabled(webStr.isEmpty());
    copyEncAction->setDisabled(downStr.isEmpty());

    itemContextMenu->popup(itemTable->viewport()->mapToGlobal(pos));
}

void lightrss::refreshImage(int fIndex)
{
    if (fIndex < 0) {
        QTableWidgetItem *twi = feedTable->currentItem();
        if (!twi) return;
        fIndex = twi->data(IdRole).toInt();
    }

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return;

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return;

    QString xmlStr = getFeedXml(fIndex);
    if (xmlStr.isEmpty()) return;

    QFile file(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlStr));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return;
    }

    QDomDocument doc("rss");
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }

    file.close();

    QDomNodeList channel = doc.elementsByTagName("channel");
    if (channel.length() < 1) return;

    QString imgUrl = getXMLValue(channel.at(0), templates[tIndex].feedImage);
    if (imgUrl.isEmpty()) return;

    startDownload(imgUrl, fIndex);
}

bool lightrss::clearCatalogEntry(int index)
{
    int success = 0;
    if (setFeedXml(index, "")) success++;
    if (setFeedUrl(index, "")) success++;
    if (setFeedImg(index, "")) success++;
    if (setFeedTpl(index, "")) success++;
    return (success == 4);
}

void lightrss::deleteFeed()
{
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();

    QString xmlStr = getFeedXml(index);
    if (xmlStr.isEmpty()) return;

    QString imgStr = getFeedImg(index);

    // we only clear the catalog entry values.
    // we don't remove the entry from the list
    // because it would mess up the index that
    // we assign to each table item. we need
    // the total number of entries to remain
    // the same in order for the assigned index
    // on each table item to remain accurate.

    if (!clearCatalogEntry(index)) return;

    int row = feedTable->currentRow();
    int col = feedTable->currentColumn();
    feedTable->clearSelection();
    twi = feedTable->takeItem(row, col);
    delete twi;

    QFile feed(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlStr));
    if (feed.exists()) feed.remove();
    if (!imgStr.isEmpty()) {
        QFile image(tr("%1%2%3").arg(imagesPath).arg(sep).arg(imgStr));
        if (image.exists()) image.remove();
    }

    feedTable->shiftCells(0, 0, feedTable->rowCount() - 1, feedTable->columnCount() - 1);
}

void lightrss::copyFeedUrl()
{
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();
    QString urlStr = getFeedUrl(index);
    if (urlStr.isEmpty()) return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(urlStr);
}

void lightrss::copyItemUrl()
{
    QTableWidgetItem *itwi = itemTable->currentItem();
    if (!itwi) return;

    int iIndex = itwi->data(IdRole).toInt();

    QTableWidgetItem *ftwi = feedTable->currentItem();
    if (!ftwi) return;

    int fIndex = ftwi->data(IdRole).toInt();

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return;

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return;

    QString webStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemWebpage);
    if (webStr.isEmpty()) return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(webStr);
}

void lightrss::copyEnclosureUrl()
{
    QTableWidgetItem *itwi = itemTable->currentItem();
    if (!itwi) return;

    int iIndex = itwi->data(IdRole).toInt();

    QTableWidgetItem *ftwi = feedTable->currentItem();
    if (!ftwi) return;

    int fIndex = ftwi->data(IdRole).toInt();

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return;

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return;

    QString downStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemDownload);
    if (downStr.isEmpty()) return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(downStr);
}

void lightrss::clearItemTable()
{
    itemTable->clearSelection();
    while (itemTable->rowCount() > 0) {
        itemTable->removeRow(0);
    }
}

int lightrss::getTemplateIndex(QString tpl)
{
    for (int i = 0; i < templates.length(); i++) {
        if (templates[i].tplName == tpl) return i;
    }
    return -1;
}

QString lightrss::getXMLValue(const QDomNode &srcNode, QList<QStringList> list)
{
    bool isError;
    QString attStr, xmlVal = "";

    QDomNode node;
    for (int i = 0; i < list.length(); i++) {
        isError = 0;
        attStr = "";
        xmlVal = "";
        node = srcNode;

        for (int j = 0; j < list[i].length(); j++) {
            isError = node.isNull() || !node.isElement();
            if (isError) break;

            if (list[i][j].startsWith("@")) {
                attStr = list[i][j].mid(1);
                break;
            }

            node = node.namedItem(list[i][j]);
        }

        if (isError) continue;

        if (!attStr.isEmpty()) {
            xmlVal = node.toElement().attribute(attStr);
        } else {
            xmlVal = node.toElement().text();
        }

        if (!xmlVal.isEmpty()) return xmlVal;
    }

    return xmlVal;
}

void lightrss::selectItem()
{
    int iIndex = itemTable->currentRow();
    if (iIndex < 0) return;

    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    int fIndex = twi->data(IdRole).toInt();

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return;

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return;

    double sizeInMb;
    QString minStr;

    // reset labels in case an element is missing from
    // one of the items. this will cause the labels to
    // be hidden instead of showing the previous value.
    linkLabel->clear();
    encLabel->clear();
    sizeLabel->clear();
    durationLabel->clear();
    descBrowser->clear();

    QString webStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemWebpage);
    if (!webStr.isEmpty()) {
        linkLabel->setText(tr("<a href=\"%1\" target=\"_blank\">Webpage</a>").arg(webStr));
        linkLabel->setStatusTip(webStr);
    }

    QString downStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemDownload);
    if (!downStr.isEmpty()) {
        encLabel->setText(tr("<a href=\"%1\" target=\"_blank\">Download</a>").arg(downStr));
        encLabel->setStatusTip(downStr);
    }

    QString sizeStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemSize);
    if (!sizeStr.isEmpty() && sizeStr != "0") {
        sizeInMb = sizeStr.toInt() / 1000000.0;
        sizeStr = QString::number(sizeInMb, 'f', 2);
        sizeLabel->setText(tr("%1 mb").arg(sizeStr));
    }

    QString durStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemDuration);
    if (!durStr.isEmpty()) {
        minStr = convertDuration(durStr);
        durationLabel->setText(tr("%1 min").arg(minStr));
    }

    QString descStr = getXMLValue(feedItems.at(iIndex), templates[tIndex].itemDescription);
    if (!descStr.isEmpty()) {
        descBrowser->setHtml(descStr);
    }
}

QString lightrss::convertDuration(QString duration)
{
    int minutes = 0;
    QStringList timeList;

    if (duration.contains(":")) {
        timeList = duration.split(":");
    } else {
        timeList << duration;
    }

    // there shouldn't be more than 3 values in timeList
    if (timeList.length() > 3) return "0";

    for (int i = timeList.length() - 1; i > -1; i--) {
        if (i == timeList.length() - 1) { // seconds
            minutes = timeList[i].toInt() / 60;
            // we round up for any left over seconds
            minutes++;
        } else if (i == timeList.length() - 2) { // minutes
            minutes += timeList[i].toInt();
        } else { // hours
            minutes += timeList[i].toInt() * 60;
        }
    }

    return QString::number(minutes);
}

void lightrss::selectFeed(QTableWidgetItem *twi)
{
    if (!twi) {
        twi = feedTable->currentItem();
        if (!twi) return;
    }

    int fIndex = twi->data(IdRole).toInt();

    QString tpl = getFeedTpl(fIndex);
    if (tpl.isEmpty()) return;

    int tIndex = getTemplateIndex(tpl);
    if (tIndex < 0) return;

    clearItemTable();
    titleLabel->clear();
    linkLabel->clear();
    encLabel->clear();
    sizeLabel->clear();
    durationLabel->clear();
    descBrowser->clear();

    int row;
    QString xmlStr, feedTitleStr, itemTitleStr, dateStr;
    QDateTime dt;

    xmlStr = getFeedXml(fIndex);
    if (xmlStr.isEmpty()) {
        qDebug() << "xml filename is not specified";
        return;
    }

    QFile file(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlStr));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return;
    }

    QDomDocument doc("rss");
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }

    file.close();

    QDomNodeList channel = doc.elementsByTagName("channel");
    if (channel.length() < 1) {
        qDebug() << "cannot find channel element in rss feed";
        return;
    }

    feedTitleStr = getXMLValue(channel.at(0), templates[tIndex].feedTitle);
    if (!feedTitleStr.isEmpty()) {
        feedTitleStr = feedTitleStr.remove(QRegExp("\\|.+$"));
        feedTitleStr = feedTitleStr.trimmed();
    } else {
        feedTitleStr = xmlStr;
    }

    titleLabel->setText(feedTitleStr);

    feedItems = doc.elementsByTagName("item");

    for (int i = 0; i < feedItems.length(); i++) {
        itemTitleStr = getXMLValue(feedItems.at(i), templates[tIndex].itemTitle);
        if (itemTitleStr.isEmpty()) continue; // we need a title to create the table item

        row = itemTable->rowCount();
        itemTable->insertRow(row);

        QTableWidgetItem *titleItem = new QTableWidgetItem;
        titleItem->setText(itemTitleStr);
        titleItem->setSizeHint(QSize(820, 24));
        titleItem->setData(IdRole, i);
        itemTable->setItem(row, 0, titleItem);

        dateStr = getXMLValue(feedItems.at(i), templates[tIndex].itemDate);
        if (!dateStr.isEmpty()) {
            dt = QDateTime::fromString(dateStr, Qt::RFC2822Date);
            QTableWidgetItem *dateItem = new QTableWidgetItem;
            dateItem->setText((!dt.isNull() && dt.isValid()) ? dt.toString("ddd MM/dd/yyyy hh:mm:ss A") : dateStr);
            dateItem->setData(IdRole, i);
            dateItem->setTextAlignment(Qt::AlignCenter);
            itemTable->setItem(row, 1, dateItem);
        }
    }

    itemTable->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    backAction->setDisabled(0);
    tableStack->setCurrentIndex(1);
}

void lightrss::showFeeds()
{
    backAction->setDisabled(1);
    tableStack->setCurrentIndex(0);
}

int lightrss::createCatalogEntry(QString xml, QString url, QString img, QString tpl)
{
    catalogList << (QStringList() << xml << url << img << tpl);

    return catalogList.length() - 1;
}

void lightrss::saveXML()
{
    QFile file(catalogPath);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qDebug() << file.errorString();
        return;
    }

    int index;
    QString imgStr, xmlStr, urlStr, tplStr;
    QFileInfo finfo;
    QTableWidgetItem *twi;
    QXmlStreamWriter xml;
    xml.setAutoFormatting(1);
    xml.setDevice(&file);
    xml.writeStartDocument();
    xml.writeStartElement("catalog");
    xml.writeStartElement("feeds");

    for (int r = 0; r < feedTable->rowCount(); r++) {
        for (int c = 0; c < feedTable->columnCount(); c++) {
            twi = feedTable->item(r, c);
            if (!twi) continue;

            index = twi->data(IdRole).toInt();

            xmlStr = getFeedXml(index);
            if (xmlStr.isEmpty()) continue;

            finfo.setFile(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlStr));
            if (!finfo.exists()) continue;

            urlStr = getFeedUrl(index);
            if (urlStr.isEmpty()) continue;

            imgStr = "";
            imgStr = getFeedImg(index);
            if (!imgStr.isEmpty()) {
                finfo.setFile(tr("%1%2%3").arg(imagesPath).arg(sep).arg(imgStr));
                if (!finfo.exists()) imgStr = "";
            }

            tplStr = "";
            tplStr = getFeedTpl(index);
            if (!tplStr.isEmpty()) {
                finfo.setFile(tr("%1%2%3").arg(templatesPath).arg(sep).arg(tplStr));
                if (!finfo.exists()) tplStr = "";
            }

            xml.writeStartElement("item");
            xml.writeTextElement("xml", xmlStr);
            xml.writeTextElement("url", urlStr);
            xml.writeTextElement("img", imgStr);
            xml.writeTextElement("tpl", tplStr);
            xml.writeEndElement();
        }
    }

    xml.writeEndDocument();
    file.close();
}

void lightrss::showEvent(QShowEvent *event)
{
    importSettings();
    event->accept();
}

void lightrss::hideEvent(QHideEvent *event)
{
    saveXML();
    saveSettings();
    event->accept();
}

void lightrss::saveSettings()
{
    QSettings settings("lightrss", "lightrss");
    settings.setValue("settings/winWidth", geometry().width());
    settings.setValue("settings/winHeight", geometry().height());
    settings.setValue("settings/winX", geometry().x());
    settings.setValue("settings/winY", geometry().y());
}

void lightrss::importSettings()
{
    QSettings settings("lightrss", "lightrss");
    resize(settings.value("settings/winWidth").toInt(),
           settings.value("settings/winHeight").toInt());
    move(settings.value("settings/winX").toInt(),
         settings.value("settings/winY").toInt());
}

TableWidget::TableWidget(QWidget *parent)
    : QTableWidget(parent)
{

}

void TableWidget::dropEvent(QDropEvent *event)
{
    QModelIndex modelIndex = indexAt(event->pos());
    event->setDropAction(Qt::IgnoreAction);

    if (!modelIndex.isValid() || selectedItems().length() < 1) return;

    QTableWidgetItem *sourceItem = selectedItems()[0];
    if (!sourceItem) return;

    int startRow = sourceItem->row();
    int startCol = sourceItem->column();

    QTableWidgetItem *destItem = itemFromIndex(modelIndex);
    if (!destItem) return;

    int endRow = destItem->row();
    int endCol = destItem->column();

    if (startRow == endRow && startCol == endCol) return;

    shiftCells(startRow, startCol, endRow, endCol, takeItem(startRow, startCol));

    event->accept();
}

void TableWidget::shiftCells(int startRow, int startCol, int endRow, int endCol, QTableWidgetItem *twiToMove)
{
    if (startRow == endRow && startCol == endCol) return;

    bool ascending = (startRow < endRow || (startRow == endRow && startCol < endCol));
    int cols = columnCount() - 1;
    int defaultStartCol = (ascending) ? 0 : cols;
    QList<QList<int>> emptyCells;
    QTableWidgetItem *twi;

    for (int r = startRow; (ascending && r <= endRow) || (!ascending && r >= endRow); (ascending) ? r++ : r--) {
        for (int c = (r == startRow) ? startCol : defaultStartCol; (ascending && c <= cols) || (!ascending && c >= 0); (ascending) ? c++ : c--) {
            if (!item(r, c)) {
                emptyCells.append(QList<int>() << r << c);
            } else if (emptyCells.length() > 0) {
                twi = takeItem(r, c);
                setItem(emptyCells[0][0], emptyCells[0][1], twi);
                emptyCells.removeAt(0);
                emptyCells.append(QList<int>() << r << c);
            }
            if (r == endRow && c == endCol) break;
        }
        if (r == endRow) break;
    }

    if (twiToMove) {
        if (!item(endRow, endCol)) {
            setItem(endRow, endCol, twiToMove);
        } else {
            insertThumbnail(twiToMove);
        }
    }
}

void TableWidget::insertThumbnail(QTableWidgetItem *twi)
{
    if (!twi) return;

    int rows = rowCount();
    int cols = columnCount();
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (!item(r, c)) {
                setItem(r, c, twi);
                return;
            }
        }
    }

    // no empty cells so we create a new row
    insertRow(rows);
    setItem(rows, 0, twi);
}

void TableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void TableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void TableWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}
