/***************************************************************************
 *   Copyright (C) 2009 by Rafael Fernández López <ereslibre@kde.org>      *
 *   @author: Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com>*
 *                                                                         *
 * This library is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU Library General Public             *
 * License version 2 as published by the Free Software Foundation.         *
 *                                                                         *
 * This library is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Library General Public License for more details.                        *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include "categorizedtreeview.h"

//Qt
#include <QtGui/QStyledItemDelegate>
#include <QtGui/QPainter>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QBrush>
#include <QEvent>

//KDE
#include <klocale.h>
#include <kdebug.h>

//SFLPhone
#include <delegates/conferencedelegate.h>
#include <klib/tipmanager.h>
#include <lib/call.h>
#include <widgets/tips/tipcollection.h>

///Constructor
CategorizedTreeView::CategorizedTreeView(QWidget *parent)
  : QTreeView(parent),m_Type(CategorizedTreeView::ViewType::Other)
{
}

///Do not draw branches
void CategorizedTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
  Q_UNUSED( painter )
  Q_UNUSED( rect    )
  Q_UNUSED( index   )
}

void CategorizedTreeView::contextMenuEvent ( QContextMenuEvent * e ) {
  const QModelIndex index = indexAt(e->pos());
  emit contextMenuRequest(index);
  e->accept();
}


void CategorizedTreeView::dragLeaveEvent( QDragLeaveEvent *e)
{
   if (m_HoverIdx.isValid()) {
      ((QAbstractItemModel*)m_HoverIdx.model())->setData(m_HoverIdx,-1,300);
      m_HoverIdx = QModelIndex();
   }
   if (TipCollection::removeConference() == TipCollection::manager()->currentTip()) {
      TipCollection::manager()->setCurrentTip(nullptr);
   }
   QTreeView::dragLeaveEvent(e);
}

void CategorizedTreeView::dragEnterEvent( QDragEnterEvent *e)
{
   const QModelIndex& idxAt = indexAt(e->pos());
   e->acceptProposedAction();
   e->accept();
   if (idxAt.isValid() && idxAt.parent().isValid()) {
      ((QAbstractItemModel*)idxAt.model())->setData(idxAt,1,300);
      m_HoverIdx = idxAt;
   }

//    QTreeView::dragEnterEvent(e);
}

void CategorizedTreeView::dragMoveEvent( QDragMoveEvent *e)
{
   const QModelIndex& idxAt = indexAt(e->pos());
   e->acceptProposedAction();
   e->accept();
   if (idxAt.isValid()) {
      if (m_HoverIdx != idxAt) {
         if (m_HoverIdx.isValid()) {
            ((QAbstractItemModel*)m_HoverIdx.model())->setData(m_HoverIdx,-1,300);
         }
         ((QAbstractItemModel*)idxAt.model())->setData(idxAt,1,300);
         m_HoverIdx = idxAt;
      }
      if (TipCollection::removeConference() == TipCollection::manager()->currentTip()) {
         TipCollection::manager()->setCurrentTip(nullptr);
      }
   }
   else if (m_Type == CategorizedTreeView::ViewType::Call) {
      if (TipCollection::removeConference() != TipCollection::manager()->currentTip() /*&& idxAt.parent().isValid()*/) {
         TipCollection::manager()->setCurrentTip(TipCollection::removeConference());
      }
   }
//    QTreeView::dragMoveEvent(e);
}

void CategorizedTreeView::setDelegate(QStyledItemDelegate* delegate)
{
   setItemDelegate(delegate);
}


void CategorizedTreeView::mouseDoubleClickEvent(QMouseEvent* event)
{
   const QModelIndex& idxAt = indexAt(event->pos());
   emit itemDoubleClicked(idxAt);
   QTreeView::mouseDoubleClickEvent(event);
}

///This function allow for custom rendering of the drag widget
void CategorizedTreeView::startDrag(Qt::DropActions supportedActions)
{
   if (m_Type != CategorizedTreeView::ViewType::Call)
      QAbstractItemView::startDrag(supportedActions);
   else {
//     Q_D(QAbstractItemView);
      const QModelIndex& index = selectionModel()->currentIndex();
      if (index.isValid()) {
         QModelIndexList list;
         list << index;
         QMimeData *data = model()->mimeData(list);
         if (!data)
            return;

         //Execute the drag
         QDrag *drag = new QDrag(this);
         drag->setPixmap(ConferenceDelegate::getDragPixmap(this,index));
         drag->setMimeData(data);
         drag->setHotSpot(QCursor::pos() - QCursor::pos());
         const Qt::DropAction defaultDropAction = Qt::IgnoreAction;
         drag->exec(supportedActions, defaultDropAction);
      }
   }
}

bool CategorizedTreeView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
   if (state() == QAbstractItemView::EditingState)
      return !(index.data(Call::Role::CallState).toInt() != size_t(Call::State::DIALING));
   return QTreeView::edit(index,trigger,event);
}
