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

#ifndef ADD_TEMPLATE_H
#define ADD_TEMPLATE_H

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QtEvents>

class AddTemplate : public QDialog
{
    Q_OBJECT

public:
    AddTemplate(QString templatesPath, QWidget *parent = nullptr);
    QLabel *tplLabel;
    QComboBox *tplMenu;
    void loadMenu();

private:
    QString tplPath;
};

#endif // ADD_TEMPLATE_H
