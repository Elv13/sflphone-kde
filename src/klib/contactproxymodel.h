/****************************************************************************
 *   Copyright (C) 2013 by Savoir-Faire Linux                               *
 *   Author : Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com> *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#ifndef CONTACTBYNAMEPROXYMODEL_H
#define CONTACTBYNAMEPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QHash>

//SFLPhone
#include "../lib/typedefs.h"
#include "../lib/contact.h"
class ContactBackend;


class LIB_EXPORT ContactByNameProxyModel :  public QAbstractItemModel
{
   Q_OBJECT
public:
   friend class ContactBackend;
   ContactByNameProxyModel(ContactBackend* parent);

   //Model implementation
   virtual bool          setData     ( const QModelIndex& index, const QVariant &value, int role   );
   virtual QVariant      data        ( const QModelIndex& index, int role = Qt::DisplayRole        ) const;
   virtual int           rowCount    ( const QModelIndex& parent = QModelIndex()                   ) const;
   virtual Qt::ItemFlags flags       ( const QModelIndex& index                                    ) const;
   virtual int           columnCount ( const QModelIndex& parent = QModelIndex()                   ) const;
   virtual QModelIndex   parent      ( const QModelIndex& index                                    ) const;
   virtual QModelIndex   index       ( int row, int column, const QModelIndex& parent=QModelIndex()) const;
   virtual QVariant      headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

private:
   class TopLevelItem : public ContactTreeBackend,public QObject {
   friend class ContactByNameProxyModel;
   public:
      virtual QObject* getSelf() {return this;}
   private:
      TopLevelItem(QString name) : ContactTreeBackend(ContactTreeBackend::TOP_LEVEL),QObject(nullptr),m_Name(name) {}
      int counter;
      int idx;
      QList<Contact*> m_lChilds;
      QString m_Name;
   };
   virtual ~ContactByNameProxyModel();
   ContactBackend* m_pModel;
   QList<TopLevelItem*> m_lCategoryCounter;
   QHash<QString,TopLevelItem*> m_hCategories;
   
   QModelIndex getContactIndex(Contact* ct) const;

private Q_SLOTS:
   void reloadCategories();

protected:
//    virtual bool lessThan ( const QModelIndex & left, const QModelIndex & right ) const;
};

#endif