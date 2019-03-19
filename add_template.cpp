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

#include "add_template.h"

AddTemplate::AddTemplate(QString templatesPath, QWidget *parent)
    : QDialog(parent)
{
    int winWidth = 250;
    int winHeight = 120;

    tplPath = templatesPath;

    tplLabel = new QLabel;
    tplLabel->setTextFormat(Qt::RichText);

    tplMenu = new QComboBox;
    
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(tplMenu, 0, Qt::AlignCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tplLabel, 0, Qt::AlignCenter);
    mainLayout->addLayout(hLayout);
    setLayout(mainLayout);

    setModal(1);
    setWindowTitle("Add template");
    setWindowIcon(QIcon(":/images/addtemplate48.png"));
    resize(winWidth, winHeight);
}

void AddTemplate::loadMenu()
{
    QDir tplDir(tplPath);
    QStringList templates;
    templates << "" << "default.xml";
    templates << tplDir.entryList(QStringList() << "*.tpl" << "*.xml",
                                  QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable,
                                  QDir::Name | QDir::IgnoreCase);
    tplMenu->clear();
    tplMenu->addItems(templates);
}
