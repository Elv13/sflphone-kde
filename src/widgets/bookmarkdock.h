/***************************************************************************
 *   Copyright (C) 2009-2015 by Savoir-Faire Linux                         *
 *   Author : Emmanuel Lepage Valle <emmanuel.lepage@savoirfairelinux.com >*
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/
#ifndef BOOKMARK_DOCK_H
#define BOOKMARK_DOCK_H

#include <QtWidgets/QDockWidget>
#include <QtCore/QSortFilterProxyModel>
#include "ui_dockbase.h"

//Qt
class QCheckBox;

//KDE
class QAction;
class BookmarkSortFilterProxyModel;

//Ring
#include <categorizedbookmarkmodel.h>

///BookmarkDock: Dock for managing favorite contacts
class BookmarkDock : public QDockWidget, public Ui_DockBase {
   Q_OBJECT
public:
   //Constructors
   explicit BookmarkDock(QWidget* parent = nullptr);
   virtual ~BookmarkDock();
private:
   //Attributes
   QCheckBox*              m_pMostUsedCK ;
   QMenu*                  m_pMenu       ;
   QModelIndex             m_CurrentIndex;
   BookmarkSortFilterProxyModel* m_pProxyModel;

   //Actions
   QAction * m_pBookmark    ;
   QAction * m_pCallAgain   ;
   QAction * m_pEditPerson  ;
   QAction * m_pCopy        ;
   QAction * m_pEmail       ;
   QAction * m_pAddPhone    ;

private Q_SLOTS:
   void reload();
   void expandTree();
   void slotDoubleClick(const QModelIndex& index);
   void slotContextMenu( const QModelIndex& index );

   //Menu
   void removeBookmark();
   void callAgain   ();
   void addPhone    ();
   void sendEmail   ();
   void copy        ();
   void editPerson ();
};

class BookmarkSortFilterProxyModel : public QSortFilterProxyModel
{
   Q_OBJECT
public:
   explicit BookmarkSortFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}
protected:
   virtual bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const
   {
      if (!source_parent.isValid() ) { //Is a category
         for (int i=0;i<CategorizedBookmarkModel::instance()->rowCount(CategorizedBookmarkModel::instance()->index(source_row,0,source_parent));i++) {
            if (filterAcceptsRow(i, CategorizedBookmarkModel::instance()->index(source_row,0,source_parent)))
               return true;
         }
      }

      return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
   }
};

#endif
