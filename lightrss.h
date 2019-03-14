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

class Template
{

public:
    Template(QString tpl = "");
    QString tplName;
    QList<QStringList> feedTitle;
    QList<QStringList> feedImage;
    QList<QStringList> itemTitle;
    QList<QStringList> itemDate;
    QList<QStringList> itemWebpage;
    QList<QStringList> itemDownload;
    QList<QStringList> itemSize;
    QList<QStringList> itemDuration;
    QList<QStringList> itemDescription;
};

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
    enum MapRoles { FeedTitle = 0, FeedImage, ItemTitle,
                    ItemDate, ItemWebpage, ItemDownload,
                    ItemSize, ItemDuration, ItemDescription };

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

    QStringList mapList;
    QList<Template> templates;

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
    void connectEvents();
    void loadFeeds();
    void loadFeedTable();
    void clearItemTable();
    void clearFeedTable();
    void saveSettings();
    void importSettings();
    void loadTemplate(QString tpl);
    void startDownload(const QUrl &url);
    void insertThumbnail(QTableWidgetItem *twi);
    void updateThumbnail(QString feedTitle, QString fext);

    bool clearCatalogEntry(int index);
    bool isHttpRedirect(QNetworkReply *reply);
    bool setFeedXml(int index, QString value);
    bool setFeedUrl(int index, QString value);
    bool setFeedImg(int index, QString value);
    bool setFeedTpl(int index, QString value);
    bool saveToDisk(const QString &path, const QString &filename, QIODevice *data);

    int getTemplateIndex(QString tpl);
    int createCatalogEntry(QString xml, QString url, QString img = "", QString tpl = "");

    QString getFeedUrl(int index);
    QString getFeedXml(int index);
    QString getFeedImg(int index);
    QString getFeedTpl(int index);
    QString getFileName(const QUrl &url);
    QString convertDuration(QString duration);
    QString extractFeedUrl(QNetworkReply *reply);
    QString getFeedTitle(const QString &filename);
    QString getFeedTitleForImage(const QString &url);
    QString getItemValue(int index, QList<QStringList> list);

    QTableWidgetItem* createFeedTableItem(int index);

private slots:
    void saveXML();
    void addFeed();
    void showFeeds();
    void selectItem();
    void deleteFeed();
    void copyFeedUrl();
    void copyItemUrl();
    void refreshFeed();
    void updateAllFeeds();
    void copyEnclosureUrl();
    void refreshImage(int index = -1);
    void feedMenuRequested(QPoint pos);
    void itemMenuRequested(QPoint pos);
    void selectFeed(QTableWidgetItem *twi);
    void downloadFinished(QNetworkReply *reply);
    void downloadError(QNetworkReply::NetworkError code);
    void downloadSslErrors(const QList<QSslError> &sslErrors);

protected:
    void hideEvent(QHideEvent *event);
    void showEvent(QShowEvent *event);
};

#endif // LIGHTRSS_H
