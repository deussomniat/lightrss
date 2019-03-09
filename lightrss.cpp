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

lightrss::lightrss(QWidget *parent)
    : QMainWindow(parent)
{
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

    acceptedXmlTypes << "application/rss+xml" << "application/x-rss+xml"
                     << "application/rdf+xml" << "application/x-rdf+xml"
                     << "application/atom+xml" << "application/x-atom+xml"
                     << "application/xml" << "application/x-xml"
                     << "text/xml";

    acceptedImgTypes << "image/jpeg" << "image/png" << "image/gif";

    acceptedXmlExtensions << "xml" << "rss";
    acceptedImgExtensions << "jpg" << "jpeg" << "gif" << "png";

    manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    createWidgets();
    loadFeeds();
    connectEvents();

    setMinimumWidth(1034);
    setMinimumHeight(472);
    setWindowTitle("lightrss");
    setWindowIcon(QIcon(":/images/rss48.png"));
}

lightrss::~lightrss()
{

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
    connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    connect(urlTextBox, SIGNAL(returnPressed()), this, SLOT(addFeed()));
}

QString lightrss::getFileName(const QUrl &url)
{
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
        basename = "download";

    if (QFile::exists(basename)) {
        // already exists, don't overwrite
        int i = 0;
        basename += '.';
        while (QFile::exists(basename + QString::number(i)))
            ++i;

        basename += QString::number(i);
    }

    return basename;
}

QString lightrss::getFeedTitleForImage(const QString &url)
{
    // [0] = index, [1] = feed title, [2] = img url

    QString imgUrl;
    for (int i = 0; i < imgTracker.length(); i++) {
        imgUrl = imgTracker[i][2].toString();
        if (url == imgUrl) return imgTracker[i][1].toString();
    }
    return "";
}

QString lightrss::getFeedTitle(const QString &filename)
{
    QFile file(tr("%1%2%3").arg(feedsPath).arg(sep).arg(filename));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return "";
    }

    QDomDocument doc("rss");
    if (!doc.setContent(&file)) {
        qDebug() << tr("Can't parse %1 xml!").arg(filename);
        file.close();
        return "";
    }

    file.close();

    QDomNodeList channel;
    QDomNode feedTitle;
    QString xmlNameStr = "";
    channel = doc.elementsByTagName("channel");
    if (channel.length() < 1) {
        qDebug() << "No channel element found in feed!";
        return "";
    }

    feedTitle = channel.at(0).namedItem("title");
    if (feedTitle.isNull() || !feedTitle.isElement()) {
        qDebug() << "Feed title is invalid!";
        return "";
    }

    xmlNameStr = feedTitle.toElement().text();
    if (xmlNameStr.isEmpty()) {
        qDebug() << tr("%1 title element is empty!").arg(filename);
        return "";
    }

    xmlNameStr = xmlNameStr.toLower();
    xmlNameStr = xmlNameStr.replace(QRegExp("\\|.+$"), "");
    xmlNameStr = xmlNameStr.replace(QRegExp("[^a-z0-9_]"), "_");
    xmlNameStr = xmlNameStr.replace(QRegExp("_{2,}"), "_");
    xmlNameStr = xmlNameStr.replace(QRegExp("^_|_$"), "");
    //xmlNameStr.append(".xml");

    return xmlNameStr;
}

bool lightrss::saveToDisk(const QString &path, const QString &filename, QIODevice *data)
{
    QFile file(tr("%1%2%3").arg(path).arg(sep).arg(filename));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << tr("Could not open %1 for writing: %2").arg(filename).arg(file.errorString());
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

    startDownload(QUrl(urlStr));
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
    return (index > -1 && index < catalogList.length()) ? catalogList[index][3] : "";
}

bool lightrss::setFeedTpl(int index, QString value)
{
    if (index < 0 || index > catalogList.length() - 1) return 0;
    catalogList[index][3] = value;
    return 1;
}

void lightrss::updateAllFeeds()
{
    QString urlStr;
    for (int i = 0; i < catalogList.length(); i++) {
        urlStr = getFeedUrl(i);
        if (urlStr.isEmpty()) continue;
        startDownload(QUrl(urlStr));
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

    startDownload(QUrl(urlStr));
}

void lightrss::startDownload(const QUrl &url)
{
    if (!ncmgr.isOnline() || manager.networkAccessible() != QNetworkAccessManager::Accessible) {
        qDebug() << "The network is not available.";
        return;
    }

    if (!url.isValid()) {
        qDebug() << "The url is not valid.";
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "lightrss/0.1");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setMaximumRedirectsAllowed(3);

    QNetworkReply *reply = manager.get(request);

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#ifndef QT_NO_SSL
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(downloadSslErrors(QList<QSslError>)));
#endif

    currentDownloads.append(reply);
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
    feedUrl = feed["feedUrl"].toString();
    return feedUrl;
}

void lightrss::downloadFinished(QNetworkReply *reply)
{
    bool isError, isImage = 0, isOldFile = 0;
    int prevTotal, index = 0;
    QString mime, filename, feedTitle, feedUrl;
    QUrl url = reply->url();

    isError = reply->error();
    if (isError) {
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
        isError = (mime.isNull() || mime.isEmpty());
        if (isError) {
            qDebug() << "No mime type returned!";
        } else {
            mime = mime.toLower();
            mime = mime.replace(QRegExp(";.*$"), "");
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
        filename = getFileName(url);
        isError = !saveToDisk((isImage) ? imagesPath : feedsPath, filename, reply);
        if (isError) {
            qDebug() << tr("Cannot save %1 to disk!").arg(filename);
        }
    }

    if (!isError) {
        if (!isImage) {
            feedTitle = getFeedTitle(filename);
        } else {
            feedTitle = getFeedTitleForImage(url.toString());
        }
        isError = (feedTitle.isEmpty());
        if (isError) {
            qDebug() << "Cannot find feed title!";
        }
    }

    if (!isError) {
        QFile oldFile;
        if (!isImage) {
            oldFile.setFileName(tr("%1%2%3.xml").arg(feedsPath).arg(sep).arg(feedTitle));
        } else {
            oldFile.setFileName(tr("%1%2%3.%4").arg(imagesPath).arg(sep).arg(feedTitle).arg(fext));
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
            isError = !QFile::rename(tr("%1%2%3").arg(feedsPath).arg(sep).arg(filename),
                                     tr("%1%2%3.xml").arg(feedsPath).arg(sep).arg(feedTitle));
        } else {
            isError = !QFile::rename(tr("%1%2%3").arg(imagesPath).arg(sep).arg(filename),
                                     tr("%1%2%3.%4").arg(imagesPath).arg(sep).arg(feedTitle).arg(fext));
        }
        if (isError) {
            qDebug() << "Cannot rename updated file!";
        }
    }

    if (!isError && isImage) {
        updateThumbnail(feedTitle, fext);
    }

    if (!isError && !isImage && !isOldFile) {
        prevTotal = catalogList.length();
        index = createCatalogEntry("", tr("%1.xml").arg(feedTitle), url.toString());
        isError = (index < 0 || prevTotal == catalogList.length());
        if (isError) {
            qDebug() << "Cannot update catalog with new feed!";
        }
    }

    if (!isError && !isImage && !isOldFile) {
        QTableWidgetItem *thumbnail = createFeedTableItem(index);
        isError = (!thumbnail);
        if (isError) {
            qDebug() << "Cannot create new table item!";
        } else {
            insertThumbnail(thumbnail);
            refreshImage(index);
        }
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();

    if (currentDownloads.isEmpty()) {
        statusbar->showMessage("All downloads finished!", 5000);
    } else {
        statusbar->showMessage(tr("%1 download%2 remaining").
                               arg(QString::number(currentDownloads.length())).
                               arg((currentDownloads.length() > 1) ? "s" : ""), 5000);
    }
}

void lightrss::updateThumbnail(QString feedTitle, QString fext)
{
    // [0] = index, [1] = title, [2] = url

    int index = -1;
    QString imageTitle;
    for (int i = 0; i < imgTracker.length(); i++) {
        imageTitle = imgTracker[i][1].toString();
        if (imageTitle == feedTitle) {
            index = imgTracker[i][0].toInt();
            imgTracker.removeAt(i);
            break;
        }
    }

    if (index < 0 || index > catalogList.length() - 1) return;

    QString imgNameStr = tr("%1.%2").arg(feedTitle).arg(fext);
    if (!setFeedImg(index, imgNameStr)) return;

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
            loaded = img->load(tr("%1%2%3").arg(imagesPath).arg(sep).arg(imgNameStr));
            if (loaded) {
                twi->setData(Qt::DecorationRole, QPixmap::fromImage(*img).scaled(200, 200, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            } else {
                qDebug() << tr("%1 was not loaded!").arg(imgNameStr);
            }
        }
    }
}

void lightrss::insertThumbnail(QTableWidgetItem *twi)
{
    int rows = feedTable->rowCount();
    int cols = feedTable->columnCount();
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (!feedTable->item(r, c)) {
                feedTable->setItem(r, c, twi);
                return;
            }
        }
    }

    // no empty cells so we create a new row
    feedTable->insertRow(rows);
    feedTable->setItem(rows, 0, twi);
}

void lightrss::feedMenuRequested(QPoint pos)
{
    // if total number of feeds is not divisible
    // by 5 then there will be empty cells in
    // table. we do not want to show context menu
    // for an empty cell. try to retrieve selected
    // item and if null do not show menu.
    QTableWidgetItem *twi = feedTable->currentItem();
    if (!twi) return;

    feedContextMenu->popup(feedTable->viewport()->mapToGlobal(pos));
}

void lightrss::itemMenuRequested(QPoint pos)
{
    // it's possible that the user may right click
    // on a pubDate item that's empty. test for
    // empty cell before displaying menu.
    QTableWidgetItem *twi = itemTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();
    QDomNode itemUrl, encUrl;
    itemUrl = feedItems.at(index).namedItem("link");
    copyItemAction->setDisabled(itemUrl.isNull() || !itemUrl.isElement() || itemUrl.toElement().text().isEmpty());
    encUrl = feedItems.at(index).namedItem("enclosure");
    copyEncAction->setDisabled(encUrl.isNull() || !encUrl.isElement() || encUrl.toElement().attribute("url").isEmpty());

    itemContextMenu->popup(itemTable->viewport()->mapToGlobal(pos));
}

void lightrss::refreshImage(int index)
{
    if (index < 0) {
        QTableWidgetItem *twi = feedTable->currentItem();
        index = twi->data(IdRole).toInt();
    }

    if (index < 0 || index > catalogList.length() - 1) return;

    QString xmlStr, imgUrlStr, feedTitle;

    xmlStr = getFeedXml(index);
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

    QDomNode chanNode, imgNode, urlNode;
    QDomNodeList channel = doc.elementsByTagName("channel");
    if (channel.length() < 1) return;

    chanNode = channel.at(0);
    if (chanNode.isNull() || !chanNode.isElement()) return;

    // try <image><url> before <itunes:image>
    imgNode = chanNode.namedItem("image");
    if (!imgNode.isNull() && imgNode.isElement()) {
        urlNode = imgNode.namedItem("url");
        if (!urlNode.isNull() && urlNode.isElement()) {
            imgUrlStr = urlNode.toElement().text();
        }
    }

    if (imgUrlStr.isEmpty()) {
        imgNode = chanNode.namedItem("itunes:image");
        if (imgNode.isNull() || !imgNode.isElement()) return;

        imgUrlStr = imgNode.toElement().attribute("href");
        if (imgUrlStr.isEmpty()) return;
    }

    feedTitle = xmlStr.replace(QRegExp("\\.xml$"), "");

    imgTracker.append(QVariantList() << index << feedTitle << imgUrlStr);
    startDownload(QUrl(imgUrlStr));
}

bool lightrss::clearCatalogEntry(int index)
{
    int success = 0;
    (setFeedXml(index, "")) ? success++ : success--;
    (setFeedUrl(index, "")) ? success++ : success--;
    (setFeedImg(index, "")) ? success++ : success--;
    (setFeedTpl(index, "")) ? success++ : success--;
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
    QTableWidgetItem *twi = itemTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();
    QDomNode link;
    link = feedItems.at(index).namedItem("link");
    if (link.isNull() || !link.isElement()) return;
    QString url = link.toElement().text();
    if (url.isEmpty()) return;
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(url);
}

void lightrss::copyEnclosureUrl()
{
    QTableWidgetItem *twi = itemTable->currentItem();
    if (!twi) return;

    int index = twi->data(IdRole).toInt();
    QDomNode enc;
    enc = feedItems.at(index).namedItem("enclosure");
    if (enc.isNull() || !enc.isElement()) return;
    QString url = enc.toElement().attribute("url");
    if (url.isEmpty()) return;
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(url);
}

void lightrss::clearItemTable()
{
    itemTable->clearSelection();
    while (itemTable->rowCount() > 0) {
        itemTable->removeRow(0);
    }
}

void lightrss::clearFeedTable()
{
    feedTable->clearSelection();
    while (feedTable->rowCount() > 0) {
        feedTable->removeRow(0);
    }
}

void lightrss::selectItem()
{
    int index = itemTable->currentRow();
    if (index < 0) return;

    double sizeInMb;
    QDomNode desc, link, enclosure, size, duration;
    QString linkTxt, enclosureTxt, sizeTxt, durationTxt, minutes;

    // reset labels in case an element is missing from
    // one of the items. this will cause the labels to
    // be hidden instead of showing the previous value.
    linkLabel->clear();
    encLabel->clear();
    sizeLabel->clear();
    durationLabel->clear();
    descBrowser->clear();

    link = feedItems.at(index).namedItem("link");
    if (!link.isNull() && link.isElement()) {
        linkTxt = link.toElement().text();
        if (!linkTxt.isEmpty()) {
            linkLabel->setText(tr("<a href=\"%1\" target=\"_blank\">Webpage</a>").arg(linkTxt));
            linkLabel->setStatusTip(linkTxt);
        }
    }

    enclosure = feedItems.at(index).namedItem("enclosure");
    if (!enclosure.isNull() && enclosure.isElement()) {
        enclosureTxt = enclosure.toElement().attribute("url");
        if (!enclosureTxt.isEmpty()) {
            encLabel->setText(tr("<a href=\"%1\" target=\"_blank\">Download</a>").arg(enclosureTxt));
            encLabel->setStatusTip(enclosureTxt);
        }
    }

    size = feedItems.at(index).namedItem("enclosure");
    if (!size.isNull() && size.isElement()) {
        sizeTxt = size.toElement().attribute("length");
        if (!sizeTxt.isEmpty() && sizeTxt != "0") {
            sizeInMb = sizeTxt.toInt() / 1000000.0;
            sizeTxt = QString::number(sizeInMb, 'f', 2);
            sizeLabel->setText(tr("%1 mb").arg(sizeTxt));
        }
    }

    duration = feedItems.at(index).namedItem("itunes:duration");
    if (!duration.isNull() && duration.isElement()) {
        durationTxt = duration.toElement().text();
        if (!durationTxt.isEmpty()){
            minutes = convertDuration(durationTxt);
            durationLabel->setText(tr("%1 min").arg(minutes));
        }
    }

    desc = feedItems.at(index).namedItem("content:encoded");
    if (desc.isNull() || !desc.isElement()) {
        desc = feedItems.at(index).namedItem("itunes:summary");
        if (desc.isNull() || !desc.isElement()) {
            desc = feedItems.at(index).namedItem("description");
            if (desc.isNull() || !desc.isElement()) {
                descBrowser->clear();
                return;
            }
        }
    }
    descBrowser->setHtml(desc.toElement().text());
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
    if (!twi) return;
    int index = twi->data(IdRole).toInt();

    clearItemTable();
    titleLabel->clear();
    linkLabel->clear();
    encLabel->clear();
    sizeLabel->clear();
    durationLabel->clear();
    descBrowser->clear();

    int row;
    QString xmlNameStr, feedTitleStr, titleStr, pubDateStr;
    QDomNode feedTitle, title, pubDate;
    QDomNodeList channel;
    QDateTime dt;

    xmlNameStr = getFeedXml(index);
    if (xmlNameStr.isEmpty()) {
        qDebug() << "xml filename is not specified";
        return;
    }

    QFile file(tr("%1%2%3").arg(feedsPath).arg(sep).arg(xmlNameStr));
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

    channel = doc.elementsByTagName("channel");
    if (channel.length() < 1) {
        qDebug() << "cannot find channel element in rss feed";
        return;
    }

    feedTitle = channel.at(0).namedItem("title");
    if (feedTitle.isNull() || !feedTitle.isElement()) {
        qDebug() << "cannot find title element in rss feed";
        return;
    }

    feedTitleStr = feedTitle.toElement().text();
    if (!feedTitleStr.isEmpty()) {
        feedTitleStr = feedTitleStr.replace(QRegExp("\\|.+$"), "");
        feedTitleStr = feedTitleStr.trimmed();
    } else {
        feedTitleStr = xmlNameStr;
    }

    titleLabel->setText(feedTitleStr);

    feedItems = doc.elementsByTagName("item");

    for (int i = 0; i < feedItems.length(); i++) {
        title = feedItems.at(i).namedItem("title");
        if (title.isNull() || !title.isElement()) continue;

        titleStr = title.toElement().text();
        if (titleStr.isEmpty()) continue;

        row = itemTable->rowCount();
        itemTable->insertRow(row);

        QTableWidgetItem *titleItem = new QTableWidgetItem;
        titleItem->setText(titleStr);
        titleItem->setSizeHint(QSize(730, 24));
        titleItem->setData(IdRole, i);
        itemTable->setItem(row, 0, titleItem);

        pubDate = feedItems.at(i).namedItem("pubDate");
        if (!pubDate.isNull() && pubDate.isElement()) {
            pubDateStr = pubDate.toElement().text();
            if (!pubDateStr.isEmpty()) {
                dt = QDateTime::fromString(pubDateStr, Qt::RFC2822Date);
                QTableWidgetItem *dateItem = new QTableWidgetItem;
                dateItem->setText((!dt.isNull() && dt.isValid()) ? dt.toString("ddd MM/dd/yyyy hh:mm:ss A") : pubDateStr);
                dateItem->setSizeHint(QSize(274, 24));
                dateItem->setData(IdRole, i);
                dateItem->setTextAlignment(Qt::AlignCenter);
                itemTable->setItem(row, 1, dateItem);
            }
        }
    }

    itemTable->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    tableStack->setCurrentIndex(1);
}

void lightrss::showFeeds()
{
    tableStack->setCurrentIndex(0);
}

void lightrss::createWidgets()
{
    backAction = new QAction(QIcon(":/images/back48.png"), "");
    backAction->setStatusTip(tr("Back"));
    addFeedAction = new QAction(QIcon(":/images/add48.png"), "");
    addFeedAction->setStatusTip(tr("Add new feed"));
    updateAllAction = new QAction(QIcon(":/images/update48.png"), "");
    updateAllAction->setStatusTip(tr("Update all feeds"));
    saveAction = new QAction(QIcon(":/images/save48.png"), "");
    saveAction->setStatusTip(tr("Save catalog"));

    deleteFeedAction = new QAction(QIcon(":/images/delete48.png"), tr("Delete"));
    refreshImageAction = new QAction(QIcon(":/images/image48.png"), tr("Refresh image"));
    refreshFeedAction = new QAction(QIcon(":/images/feed48.png"), tr("Refresh feed"));
    copyFeedAction = new QAction(QIcon(":/images/copy_feed48.png"), tr("Copy feed url"));

    copyItemAction = new QAction(QIcon(":/images/copy_item48.png"), tr("Copy item url"));
    copyEncAction = new QAction(QIcon(":/images/copy_enclosure48.png"), tr("Copy enclosure url"));

    urlTextBox = new QLineEdit;

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
    feedContextMenu->addAction(deleteFeedAction);

    itemContextMenu = new QMenu;
    itemContextMenu->addAction(copyItemAction);
    itemContextMenu->addAction(copyEncAction);

    feedTable = new TableWidget;
    feedTable->setShowGrid(false);
    feedTable->setColumnCount(5);
    feedTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    feedTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    feedTable->verticalHeader()->setVisible(false);
    feedTable->horizontalHeader()->setVisible(false);
    feedTable->horizontalHeader()->setStretchLastSection(false);
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

    titleLabel = new QLabel;
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: 700; }");
    encLabel = new QLabel;
    encLabel->setOpenExternalLinks(true);
    linkLabel = new QLabel;
    linkLabel->setOpenExternalLinks(true);
    sizeLabel = new QLabel;
    durationLabel = new QLabel;

    descBrowser = new QTextBrowser;
    descBrowser->setOpenLinks(true);
    descBrowser->setOpenExternalLinks(true);
    descBrowser->setStyleSheet("QTextBrowser { background:#ffffff; }");

    itemTable = new QTableWidget;
    itemTable->setColumnCount(2);
    itemTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    itemTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    itemTable->verticalHeader()->setVisible(false);
    itemTable->horizontalHeader()->setVisible(true);
    itemTable->horizontalHeader()->setStretchLastSection(false);
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

    tableStack = new QStackedWidget;
    tableStack->addWidget(feedTable);
    tableStack->addWidget(itemWidget);

    setCentralWidget(tableStack);
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
                if (!finfo.exists()) tplStr = "";
            }
        }

        catalogList << (QStringList() << xmlStr << urlStr << imgStr << tplStr);
    }

    loadFeedTable();
}

int lightrss::createCatalogEntry(QString img, QString xml, QString url)
{
    catalogList << (QStringList() << xml << url << img << "");

    return catalogList.length() - 1;
}

QTableWidgetItem* lightrss::createFeedTableItem(int index)
{
    bool loaded, hasImage;
    QFileInfo finfo;
    QDomNode imgName, xmlName, url;
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
    hasImage = (!imgNameStr.isEmpty());
    if (hasImage) {
        imgNameStr = tr("%1%2%3").arg(imagesPath).arg(sep).arg(imgNameStr);
        finfo.setFile(imgNameStr);
        hasImage = (finfo.exists());
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

    thumbnail->setStatusTip(urlStr);
    thumbnail->setText(xmlNameStr);
    thumbnail->setTextAlignment(Qt::AlignCenter);
    thumbnail->setData(IdRole, index);
    return thumbnail;
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

    if (twiToMove && !item(endRow, endCol)) {
        setItem(endRow, endCol, twiToMove);
    }
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
