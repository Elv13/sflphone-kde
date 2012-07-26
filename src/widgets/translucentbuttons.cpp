/***************************************************************************
 *   Copyright (C) 2011 by Savoir-Faire Linux                              *
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
#include "translucentbuttons.h"

#include <QtGui/QPainter>
#include <KDebug>

#include <QtCore/QTimer>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QMimeData>

///Constructor
TranslucentButtons::TranslucentButtons(QWidget* parent):QPushButton(parent),m_enabled(true),m_pTimer(0),m_CurrentState(0),m_pImg(0)
{
   setAcceptDrops(true);
   m_CurrentColor = "black";
   m_CurrentColor.setAlpha(0);
}

///Destructor
TranslucentButtons::~TranslucentButtons()
{
   if (m_pTimer) delete m_pTimer;
   if (m_pImg) delete m_pImg;
}

///How to paint
void TranslucentButtons::paintEvent(QPaintEvent* event)
{
   Q_UNUSED(event)
   QPainter customPainter(this);
   customPainter.setRenderHint(QPainter::Antialiasing, true);
   customPainter.setBackground(m_CurrentColor);
   customPainter.setBrush(m_CurrentColor);
   customPainter.setPen(Qt::NoPen);
   customPainter.drawRoundedRect(rect(), 10, 10);
   customPainter.setPen(m_Pen);

   if (m_pImg) {
      customPainter.drawImage(QRect(QPoint(rect().x()+rect().width()-parentWidget()->height(),10),QSize(parentWidget()->height()-10,rect().height()-20)),*m_pImg, QRectF(m_pImg->rect()));
   }

   QFont font = customPainter.font();
   font.setBold(true);
   customPainter.setFont(font);
   customPainter.drawText (rect(), Qt::AlignVCenter|Qt::AlignHCenter, text().remove('&') );
}

///Override the visibility toggler
void TranslucentButtons::setVisible(bool enabled)
{
   if (m_enabled != enabled) {
      if (m_pTimer) {
         m_pTimer->stop();
         disconnect(m_pTimer);
      }
      m_pTimer = new QTimer(this); //TODO LEAK
      connect(m_pTimer, SIGNAL(timeout()), this, SLOT(changeVisibility()));
      m_step = 0;
      m_CurrentColor = "black";
      m_CurrentColor.setAlpha(0);
      repaint();
      m_pTimer->start(10);
      raise();
   }
   m_enabled = enabled;
   QWidget::setVisible(enabled);
}

///Step by step animation
void TranslucentButtons::changeVisibility()
{
   m_step++;
   m_CurrentColor.setAlpha(0.1*m_step*m_step);
   repaint();
   if (m_step >= 35)
      m_pTimer->stop();
}

void TranslucentButtons::dragEnterEvent ( QDragEnterEvent *e )
{
   e->ignore();
}

void TranslucentButtons::dragMoveEvent  ( QDragMoveEvent  *e )
{
   e->ignore();
}

void TranslucentButtons::dragLeaveEvent ( QDragLeaveEvent *e )
{
   e->ignore();
}

///Propagate the mime data
///@note This propagate like this: button -> tree item -> treewidget
void TranslucentButtons::dropEvent(QDropEvent *e)
{
   kDebug() << "Drop accepted";
   emit dataDropped((QMimeData*)e->mimeData());
}

///Set the state when the user hover the widget
///@note This is not called directly to avoid a Qt bug/limitation
void TranslucentButtons::setHoverState(bool hover)
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
      repaint();
      m_CurrentState = hover;
   }
}

///Set the button pixmap
void TranslucentButtons::setPixmap(QImage* img)
{
   m_pImg = img;
}