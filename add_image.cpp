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

#include "add_image.h"

AddImage::AddImage(QWidget *parent)
    : QDialog(parent)
{
    int winWidth = 250;
    int winHeight = 100;

    fileBtn = new QPushButton(tr("FILE"));
    urlBtn = new QPushButton(tr("URL"));
    
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(fileBtn, 0, Qt::AlignCenter);
    hLayout->addWidget(urlBtn, 0, Qt::AlignCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(hLayout);
    setLayout(mainLayout);

    setModal(1);
    setWindowTitle("Add image");
    setWindowIcon(QIcon(":/images/addimage48.png"));
    resize(winWidth, winHeight);
}
