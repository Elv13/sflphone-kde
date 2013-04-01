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
#include "bookmarkmodel.h"

//SFLPhone
#include "configurationskeleton.h"
#include "../lib/historymodel.h"

BookmarkModel* BookmarkModel::m_pSelf = nullptr;

class NumberTreeBackend : public HistoryTreeBackend, public QObject
{
   friend class BookmarkModel;
   public:
      NumberTreeBackend(QString name): HistoryTreeBackend(HistoryTreeBackend::Type::BOOKMARK),m_Name(name),m_IsMostPopular(false){}
      virtual QObject* getSelf() { return this; }

   private:
      QString m_Name;
      bool m_IsMostPopular;
};

void BookmarkModel::reloadCategories()
{
   beginResetModel();
   m_hCategories.clear();
   foreach(TopLevelItem* item, m_lCategoryCounter) {
      foreach (NumberTreeBackend* child, item->m_lChilds) {
         delete child;
      }
      delete item;
   }
   m_lCategoryCounter.clear();
   m_isContactDateInit = false;

   //Load most used contacts
   if (ConfigurationSkeleton::displayContactCallHistory()) {
      TopLevelItem* item = new TopLevelItem("Most popular");
      m_hCategories["mp"] = item;
      m_lCategoryCounter << item;
      QStringList cl = HistoryModel::getNumbersByPopularity();
      for (int i=0;i<((cl.size()>=10)?10:cl.size());i++) {
         NumberTreeBackend* bm = new NumberTreeBackend(cl[i]);
         bm->m_IsMostPopular = true;
         item->m_lChilds << bm;
      }
   }

   foreach(QString bookmark, ConfigurationSkeleton::bookmarkList()) {
      NumberTreeBackend* bm = new NumberTreeBackend(bookmark);
      QString val = category(bm);
      if (!m_hCategories[val]) {
         TopLevelItem* item = new TopLevelItem(val);
         m_hCategories[val] = item;
         m_lCategoryCounter << item;
      }
      TopLevelItem* item = m_hCategories[val];
      if (item) {
         item->m_lChilds << bm;
      }
      else
         qDebug() << "ERROR count";
   }
   endResetModel();
   emit layoutChanged();
   emit dataChanged(index(0,0),index(rowCount()-1,0));
}

bool BookmarkModel::setData( const QModelIndex& index, const QVariant &value, int role)
{
   Q_UNUSED(index)
   Q_UNUSED(value)
   Q_UNUSED(role)
   return false;
}

QVariant BookmarkModel::data( const QModelIndex& index, int role) const
{
   if (!index.isValid())
      return QVariant();

   HistoryTreeBackend* modelItem = (HistoryTreeBackend*)index.internalPointer();
   switch (modelItem->type3()) {
      case HistoryTreeBackend::Type::TOP_LEVEL:
         switch (role) {
            case Qt::DisplayRole:
               return ((TopLevelItem*)modelItem)->m_Name;
         }
         break;
      case HistoryTreeBackend::Type::CALL:
      case HistoryTreeBackend::Type::BOOKMARK:
         return commonCallInfo(m_lCategoryCounter[index.parent().row()]->m_lChilds[index.row()],role);
         break;
      case HistoryTreeBackend::Type::NUMBER:
         break;
   };
   return QVariant();
}

QVariant BookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   Q_UNUSED(section)
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return QVariant("Contacts");
   return QVariant();
}

int BookmarkModel::rowCount( const QModelIndex& parent ) const
{
   if (!parent.isValid() || !parent.internalPointer())
      return m_lCategoryCounter.size();
   else if (!parent.parent().isValid()) {
      return m_lCategoryCounter[parent.row()]->m_lChilds.size();
   }
   return 0;
}

Qt::ItemFlags BookmarkModel::flags( const QModelIndex& index ) const
{
   if (!index.isValid())
      return 0;
   return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int BookmarkModel::columnCount ( const QModelIndex& parent) const
{
   Q_UNUSED(parent)
   return 1;
}

QModelIndex BookmarkModel::parent( const QModelIndex& index) const
{
   if (!index.isValid() || !index.internalPointer()) {
      return QModelIndex();
   }
   HistoryTreeBackend* modelItem = static_cast<HistoryTreeBackend*>(index.internalPointer());
   if (modelItem->type3() == HistoryTreeBackend::Type::BOOKMARK) {
      QString val = category(((NumberTreeBackend*)(index.internalPointer())));
      if (((NumberTreeBackend*)modelItem)->m_IsMostPopular)
         return BookmarkModel::index(0,0);
      else if (m_hCategories[val])
         return BookmarkModel::index(m_lCategoryCounter.indexOf(m_hCategories[val]),0);
      else BookmarkModel::index(0,0);
   }
   return QModelIndex();
}

QModelIndex BookmarkModel::index( int row, int column, const QModelIndex& parent) const
{
   if (parent.isValid())
      return createIndex(row,column,m_lCategoryCounter[parent.row()]->m_lChilds[row]);
   else {
      return createIndex(row,column,m_lCategoryCounter[row]);
   }
   return QModelIndex();
}

QVariant BookmarkModel::commonCallInfo(NumberTreeBackend* number, int role) const
{
   if (!number)
      return QVariant();
   QVariant cat;
   switch (role) {
      case Qt::DisplayRole:
      case HistoryModel::Role::Name:
         cat = number->m_Name;
         break;
      case HistoryModel::Role::Number:
         cat = "N/A";//call->getPeerPhoneNumber();
         break;
      case HistoryModel::Role::Direction:
         cat = 4;//call->getHistoryState();
         break;
      case HistoryModel::Role::Date:
         cat = "N/A";//call->getStartTimeStamp();
         break;
      case HistoryModel::Role::Length:
         cat = "N/A";//call->getLength();
         break;
      case HistoryModel::Role::FormattedDate:
         cat = "N/A";//QDateTime::fromTime_t(call->getStartTimeStamp().toUInt()).toString();
         break;
      case HistoryModel::Role::HasRecording:
         cat = false;//call->hasRecording();
         break;
      case HistoryModel::Role::HistoryState:
         cat = history_state::NONE;//call->getHistoryState();
         break;
      case HistoryModel::Role::Filter:
         cat = "N/A";//call->getHistoryState()+'\n'+commonCallInfo(call,Name).toString()+'\n'+commonCallInfo(call,Number).toString();
         break;
      case HistoryModel::Role::FuzzyDate:
         cat = "N/A";//timeToHistoryCategory(QDateTime::fromTime_t(call->getStartTimeStamp().toUInt()).date());
         break;
      case HistoryModel::Role::IsBookmark:
         cat = true;
         break;
   }
   return cat;
}

QString BookmarkModel::category(NumberTreeBackend* number) const
{
   QString cat = commonCallInfo(number).toString();
   if (cat.size())
      cat = cat[0].toUpper();
   return cat;
}
