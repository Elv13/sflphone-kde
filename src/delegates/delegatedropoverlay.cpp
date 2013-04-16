/***************************************************************************
 *   Copyright (C) 2011-2013 by Savoir-Faire Linux                         *
 *   Author : Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com>*
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
#include "delegatedropoverlay.h"

#include <QtGui/QPainter>
#include <KDebug>

#include <QtCore/QTimer>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QMimeData>


#include <lib/contactbackend.h>

///Constructor
DelegateDropOverlay::DelegateDropOverlay(QObject* parent):QObject(parent),
m_pTimer(0),m_CurrentState(-1),m_pImg(0),m_Init(false),m_Reverse(1)
{
   m_CurrentColor = "black";
   m_CurrentColor.setAlpha(0);
}

///Destructor
DelegateDropOverlay::~DelegateDropOverlay()
{
   if (m_pTimer) delete m_pTimer;
   if (m_pImg)   delete m_pImg;
}

///How to paint
void DelegateDropOverlay::paintEvent(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)
{
   int step = index.data(ContactBackend::Role::DropState).toInt();
   if ((step == 1 || step == -1) && m_lActiveIndexes.indexOf(index) == -1) {
      m_lActiveIndexes << index;
      //Create tge timer
      if (!m_pTimer) {
         m_pTimer = new QTimer(this);
         connect(m_pTimer, SIGNAL(timeout()), this, SLOT(changeVisibility()));
      }

      //Start it if it's nor already
      if (!m_pTimer->isActive()) {
         m_pTimer->start(10);
      }
   }
   if (step) {
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing, true);
      painter->setBackground(m_CurrentColor);
      painter->setBrush(m_CurrentColor);
      painter->setPen(Qt::NoPen);
      painter->drawRoundedRect(option.rect, 10, 10);
      painter->setPen(m_Pen);

      if (m_pImg) {
         painter->drawImage(QRect(QPoint(option.rect.x()+option.rect.width(),10),QSize(10,option.rect.height()-20)),*m_pImg, QRectF(m_pImg->rect()));
      }

      QFont font = painter->font();
      font.setBold(true);
      painter->setFont(font);
      painter->drawText (option.rect, Qt::AlignVCenter|Qt::AlignHCenter, index.data(ContactBackend::Role::DropString).toString().remove('&') );
      painter->restore();
   }
}//paintEvent

///Step by step animation
void DelegateDropOverlay::changeVisibility()
{
   foreach(QModelIndex idx, m_lActiveIndexes) {
      int step = idx.data(ContactBackend::Role::DropState).toInt();
      //Remove items from the loop if there is no animation
      if (step >= 15 || step <= -15) {
         m_lActiveIndexes.removeAll(idx);
         if (step <= -15) //Hide the overlay
            ((QAbstractItemModel*)idx.model())->setData(idx,QVariant((int)0),ContactBackend::Role::DropState);
      }
      else {
         //Update opacity
         if (step == 1)
            setHoverState(true);
         else if (step == -1)
            setHoverState(false);
         step+=(step>0)?1:-1;
         int tmpStep = (step>0)?step:15+step;
         m_CurrentColor.setAlpha(0.5*tmpStep*tmpStep); //Parabolic opacity increase steps
         ((QAbstractItemModel*)idx.model())->setData(idx,QVariant((int)step),ContactBackend::Role::DropState);
      }
   }
   //Stop loop if no animations are running
   if (!m_lActiveIndexes.size()) {
      m_pTimer->stop();
   }
}

///Set the state when the user hover the widget
///@note This is not called directly to avoid a Qt bug/limitation
void DelegateDropOverlay::setHoverState(bool hover)
{
   if (hover != m_CurrentState) {
      if (hover) {
         int alpha = m_CurrentColor.alpha();
         m_CurrentColor = "grey";
         m_CurrentColor.setAlpha(alpha);
         m_Pen.setColor("black");
      }
      else {
         int alpha = m_CurrentColor.alpha();
         m_CurrentColor = "black";
         m_CurrentColor.setAlpha(alpha);
         m_Pen.setColor("white");
      }
      m_CurrentState = hover;
   }
}//setHoverState

///Set the button pixmap
void DelegateDropOverlay::setPixmap(QImage* img)
{
   m_pImg = img;
}