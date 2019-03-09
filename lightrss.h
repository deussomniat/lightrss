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

#ifndef LIGHTRSS_H
#define LIGHTRSS_H

#include <QAction>
#include <QClipboard>
#include <QDebug>
#include <QDomDocument>
#include <QDir>
#include <QtEvents>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QtNetwork>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QToolBar>
#include <QStatusBar>
#include <QXmlStreamWriter>

class TableWidget : public QTableWidget
{
    Q_OBJECT

public:
    TableWidget(QWidget *parent = nullptr);
    void shiftCells(int startRow, int startCol, int endRow, int endCol, QTableWidgetItem *twiToMove = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
};

class lightrss : public QMainWindow
{
    Q_OBJECT

public:
    lightrss(QWidget *parent = nullptr);
    ~lightrss();
    enum DataRoles { IdRole = 0x0101 };

private:
    QString sep;
    QString homePath;
    QString tempPath;
    QString feedsPath;
    QString imagesPath;
    QString catalogPath;
    QString templatesPath;

    QStringList acceptedImgTypes;
    QStringList acceptedXmlTypes;
    QStringList acceptedXmlExtensions;
    QStringList acceptedImgExtensions;

    // [0] = xml, [1] = url, [2] = img, [3] = tpl
    QList<QStringList> catalogList;
    // [0] = index, [1] = title, [2] = url
    QList<QVariantList> imgTracker;

    QStatusBar *statusbar;

    QStackedWidget *tableStack;

    TableWidget *feedTable;
    QTableWidget *itemTable;

    QLineEdit *urlTextBox;

    QAction *backAction;
    QAction *addFeedAction;
    QAction *updateAllAction;
    QAction *saveAction;

    QAction *deleteFeedAction;
    QAction *refreshImageAction;
    QAction *refreshFeedAction;

    QAction *copyFeedAction;
    QAction *copyItemAction;
    QAction *copyEncAction;

    QMenu *feedContextMenu;
    QMenu *itemContextMenu;

    QLabel *titleLabel;
    QLabel *linkLabel;
    QLabel *encLabel;
    QLabel *sizeLabel;
    QLabel *durationLabel;

    QTextBrowser *descBrowser;

    QDomNodeList feedItems;

    QNetworkAccessManager manager;
    QNetworkConfigurationManager ncmgr;
    QVector<QNetworkReply*> currentDownloads;

    void createWidgets();
    void loadFeeds();
    void loadFeedTable();
    QTableWidgetItem* createFeedTableItem(int index);
    int createCatalogEntry(QString img, QString xml, QString url);
    void insertThumbnail(QTableWidgetItem *twi);
    void connectEvents();
    void clearItemTable();
    void clearFeedTable();
    void updateThumbnail(QString feedTitle, QString fext);
    QString convertDuration(QString duration);
    QString getFeedTitle(const QString &filename);
    QString getFeedTitleForImage(const QString &url);
    void saveSettings();
    void importSettings();
    QString getFeedUrl(int index);
    QString getFeedXml(int index);
    QString getFeedImg(int index);
    QString getFeedTpl(int index);
    bool setFeedXml(int index, QString value);
    bool setFeedUrl(int index, QString value);
    bool setFeedImg(int index, QString value);
    bool setFeedTpl(int index, QString value);
    bool clearCatalogEntry(int index);

    void startDownload(const QUrl &url);
    QString getFileName(const QUrl &url);
    bool saveToDisk(const QString &path, const QString &filename, QIODevice *data);
    bool isHttpRedirect(QNetworkReply *reply);
    QString extractFeedUrl(QNetworkReply *reply);

private slots:
    void selectFeed(QTableWidgetItem *twi);
    void selectItem();
    void feedMenuRequested(QPoint pos);
    void itemMenuRequested(QPoint pos);
    void showFeeds();
    void refreshFeed();
    void updateAllFeeds();
    void refreshImage(int index = -1);
    void copyFeedUrl();
    void copyItemUrl();
    void copyEnclosureUrl();
    void deleteFeed();
    void saveXML();
    void addFeed();
    void downloadFinished(QNetworkReply *reply);
    void downloadSslErrors(const QList<QSslError> &sslErrors);
    void downloadError(QNetworkReply::NetworkError code);

protected:
    void hideEvent(QHideEvent *event);
    void showEvent(QShowEvent *event);
};

#endif // LIGHTRSS_H
