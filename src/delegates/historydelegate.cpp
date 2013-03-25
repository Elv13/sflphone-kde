/****************************************************************************
 *   Copyright (C) 2009-2013 by Savoir-Faire Linux                          *
 *   Author : Jérémy Quentin <jeremy.quentin@savoirfairelinux.com>          *
 *            Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com> *
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
#include "historydelegate.h"

#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QTreeView>

#include <QtCore/QDebug>

#include <KLocale>
#include <KIcon>
#include <KStandardDirs>

#include <lib/historymodel.h>
#include <lib/contact.h>
#include "klib/configurationskeleton.h"
#include "widgets/playeroverlay.h"

static const char* icnPath[4] = {
/* INCOMING */ ICON_HISTORY_INCOMING,
/* OUTGOING */ ICON_HISTORY_OUTGOING,
/* MISSED   */ ICON_HISTORY_MISSED  ,
/* NONE     */ ""                   ,
};

HistoryDelegate::HistoryDelegate(QTreeView* parent) : QStyledItemDelegate(parent),m_pParent(parent)
{
}

QSize HistoryDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
   QSize sh = QStyledItemDelegate::sizeHint(option, index);
   QFontMetrics fm(QApplication::font());
   int lineHeight = fm.height()+2;
   return QSize(sh.rwidth(),(3*lineHeight)<52?52:(3*lineHeight));
}

void HistoryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
   Q_ASSERT(index.isValid());
   
   if (option.state & QStyle::State_Selected || option.state & QStyle::State_MouseOver) {
      QStyleOptionViewItem opt2 = option;
      QPalette pal = option.palette;
      pal.setBrush(QPalette::Text,QColor(0,0,0,0));
      pal.setBrush(QPalette::HighlightedText,QColor(0,0,0,0));
      opt2.palette = pal;
      QStyledItemDelegate::paint(painter,opt2,index);
   }

   painter->setPen(QApplication::palette().color(QPalette::Active,(option.state & QStyle::State_Selected)?QPalette::HighlightedText:QPalette::Text));
//    Call* call = (Call*)((HistoryTreeBackend*)((static_cast<const QSortFilterProxyModel*>(index.model()))->mapToSource(index).internalPointer()))->getSelf();
   Call* call = (Call*)static_cast<HistoryTreeBackend*>(index.internalPointer())->getSelf();
   Contact* ct = call->getContact();
   QPixmap pxm;
   if (ct && ct->getPhoto()) {
      pxm = (*ct->getPhoto());
      QRect pxRect = pxm.rect();
      QBitmap mask(pxRect.size());
      QPainter customPainter(&mask);
      customPainter.setRenderHint  (QPainter::Antialiasing, true   );
      customPainter.fillRect       (pxRect                ,"white" );
      customPainter.setBackground  (QColor("black")                );
      customPainter.setBrush       (QColor("black")                );
      customPainter.drawRoundedRect(pxRect,5,5);
      pxm.setMask(mask);
   }
   else {
      pxm = QPixmap(KIcon("user-identity").pixmap(QSize(48,48)));
   }
   
   if (index.data(HistoryModel::Role::HasRecording).toBool()) {
      QPainter painter(&pxm);
      QPixmap status(KStandardDirs::locate("data","sflphone-client-kde/voicemail.png"));
      status=status.scaled(QSize(24,24));
      painter.drawPixmap(pxm.width()-status.width(),pxm.height()-status.height(),status);
      if (m_pParent && m_pParent->indexWidget(index) == nullptr) {
         auto button = new PlayerOverlay(call,nullptr);
         button->setCall(call);
         m_pParent->setIndexWidget(index,button);
      }
   }
   else if (index.data(HistoryModel::Role::HistoryState).toInt() != history_state::NONE && ConfigurationSkeleton::displayHistoryStatus()) {
      QPainter painter(&pxm);
      QPixmap status(icnPath[index.data(HistoryModel::Role::HistoryState).toInt()]);
      status=status.scaled(QSize(24,24));
      painter.drawPixmap(pxm.width()-status.width(),pxm.height()-status.height(),status);
   }
   painter->drawPixmap(option.rect.x()+4,option.rect.y()+(option.rect.height()-48)/2,pxm);

   QFont font = painter->font();
   QFontMetrics fm(font);
   int currentHeight = option.rect.y()+fm.height()+2;
   font.setBold(true);
   painter->setFont(font);
   painter->drawText(option.rect.x()+15+48,currentHeight,index.data(Qt::DisplayRole).toString());
   font.setBold(false);
   painter->setFont(font);
   currentHeight +=fm.height();
   painter->drawText(option.rect.x()+15+48,currentHeight,index.data(HistoryModel::Role::FormattedDate).toString());
   currentHeight +=fm.height();
   painter->drawText(option.rect.x()+15+48,currentHeight,index.data(HistoryModel::Role::Number).toString());
   currentHeight +=fm.height();
}