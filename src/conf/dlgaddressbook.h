/************************************************************************************
 *   Copyright (C) 2009 by Savoir-Faire Linux                                       *
 *   Author : Jérémy Quentin <jeremy.quentin@savoirfairelinux.com>                  *
 *            Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com>         *
 *                                                                                  *
 *   This library is free software; you can redistribute it and/or                  *
 *   modify it under the terms of the GNU Lesser General Public                     *
 *   License as published by the Free Software Foundation; either                   *
 *   version 2.1 of the License, or (at your option) any later version.             *
 *                                                                                  *
 *   This library is distributed in the hope that it will be useful,                *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of                 *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU              *
 *   Lesser General Public License for more details.                                *
 *                                                                                  *
 *   You should have received a copy of the GNU Lesser General Public               *
 *   License along with this library; if not, write to the Free Software            *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA *
 ***********************************************************************************/

#ifndef DLGADDRESSBOOK_H
#define DLGADDRESSBOOK_H

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtGui/QListWidgetItem>

#include "ui_dlgaddressbookbase.h"

/**
	@author Jérémy Quentin <jeremy.quentin@gmail.com>
*/
class DlgAddressBook : public QWidget, public Ui_DlgAddressBookBase
{
Q_OBJECT
public:
   DlgAddressBook(QWidget *parent = 0);

   ~DlgAddressBook();

private:
   QHash<QString,QListWidgetItem*> m_mNumbertype;

public slots:
   void updateWidgets();
   void updateSettings();


};

#endif
