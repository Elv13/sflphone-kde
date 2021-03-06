/****************************************************************************
 *   Copyright (C) 2013-2014 by Savoir-Faire Linux                          *
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
#include "kdepixmapmanipulation.h"

//Qt
#include <QSize>
#include <QBuffer>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QBitmap>
#include <QtWidgets/QApplication>
#include <QtGui/QImage>
#include <QtGui/QPalette>

//KDE
#include <QIcon>
#include <KColorScheme>
#include <klocalizedstring.h>


//Ring
#include <person.h>
#include <contactmethod.h>
#include <presencestatusmodel.h>
#include <securityevaluationmodel.h>
#include <collectioninterface.h>
#include <useractionmodel.h>
#include <QStandardPaths>
#include "icons/icons.h"


const TypedStateMachine< const char* , Call::State > KDEPixmapManipulation::callStateIcons = {
   {  RingIcons::DIALING    ,
      RingIcons::INCOMING   ,
      RingIcons::RINGING    ,
      RingIcons::CURRENT    ,
      RingIcons::DIALING    ,
      RingIcons::HOLD       ,
      RingIcons::FAILURE    ,
      RingIcons::BUSY       ,
      RingIcons::TRANSFER   ,
      RingIcons::TRANSF_HOLD,
      ""              ,
      ""              ,
      RingIcons::CONFERENCE,
      "",
      "",
      "",
   }};

KDEPixmapManipulation::KDEPixmapManipulation() : QObject(),PixmapManipulationDelegate()
{

}

///When the Person is rebased (use a new data source), everything is now invalid
void KDEPixmapManipulation::clearCache()
{
   Person* c = qobject_cast<Person*>(sender());
   if (!c) return;

   //Hopefully it wont happen often
   foreach (const QByteArray& name, c->dynamicPropertyNames()) {
      if (name.left(5) == "photo")
         c->setProperty(name,QVariant());
   }
}

QVariant KDEPixmapManipulation::contactPhoto(Person* c, const QSize& size, bool displayPresence) {
   const QString hash = QString("photo2%1%2%3").arg(size.width()).arg(size.height()).arg(c->isPresent());
   QVariant preRendered = c->property(hash.toLatin1());
   if (preRendered.isValid())
      return preRendered;
   else
      connect(c,SIGNAL(rebased(Person*)),this,SLOT(clearCache()));
   const int radius = (size.height() > 35) ? 7 : 5;
   //const QPixmap pxmPtr = qvariant_cast<QPixmap>(c->photo());
   bool isTracked = displayPresence && c->isTracked();
   bool isPresent = displayPresence && c->isPresent();
   static QColor presentBrush = KStatefulBrush( KColorScheme::Window, KColorScheme::PositiveText ).brush(QPalette::Normal).color();
   static QColor awayBrush    = KStatefulBrush( KColorScheme::Window, KColorScheme::NegativeText ).brush(QPalette::Normal).color();

   QPixmap pxm;
   if (c->photo().isValid()) {
      QPixmap contactPhoto((qvariant_cast<QPixmap>(c->photo())).scaledToWidth(size.height()-6));
      pxm = QPixmap(size);
      pxm.fill(Qt::transparent);
      QPainter painter(&pxm);

      //Clear the pixmap
      painter.setCompositionMode(QPainter::CompositionMode_Clear);
      painter.fillRect(0,0,size.width(),size.height(),QBrush(Qt::white));
      painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

      //Add corner radius to the Pixmap
      QRect pxRect = contactPhoto.rect();
      QBitmap mask(pxRect.size());
      QPainter customPainter(&mask);
      customPainter.setRenderHint  (QPainter::Antialiasing, true      );
      customPainter.fillRect       (pxRect                , Qt::white );
      customPainter.setBackground  (Qt::black                         );
      customPainter.setBrush       (Qt::black                         );
      customPainter.drawRoundedRect(pxRect,radius,radius);
      contactPhoto.setMask(mask);
      painter.drawPixmap(3,3,contactPhoto);
      painter.setBrush(Qt::NoBrush);
      painter.setPen(Qt::white);
      painter.setRenderHint  (QPainter::Antialiasing, true   );
      painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
      painter.drawRoundedRect(3,3,pxm.height()-6,pxm.height()-6,radius,radius);
      painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

      //Draw the glow around pixmaps
      if (isTracked) {
         QPen pen(QApplication::palette().color(QPalette::Disabled,QPalette::Text));
         pen.setWidth(isTracked?1:2);
         if (isPresent)
            pen.setColor(presentBrush);
         else
            pen.setColor(awayBrush);
         for (int i=2;i<=7;i+=2) {
            pen.setWidth(i);
            painter.setPen(pen);
            painter.setOpacity(0.3f-(((i-2)*0.8f)/10.0f));
            painter.drawRoundedRect(3,3,pxm.height()-6,pxm.height()-6,radius,radius);
         }
      }
   }
   else {
      pxm = drawDefaultUserPixmap(size,isTracked,isPresent);
   }

   c->setProperty(hash.toLatin1(),pxm);
   return pxm;
}

QVariant KDEPixmapManipulation::callPhoto(const ContactMethod* n, const QSize& size, bool displayPresence) {
   if (n->contact()) {
      return contactPhoto(n->contact(),size,displayPresence);
   }
   else {
      bool isTracked = displayPresence && n->isTracked();
      bool isPresent = displayPresence && n->isPresent();

      return drawDefaultUserPixmap(size,isTracked,isPresent);
   }
}

QVariant KDEPixmapManipulation::callPhoto(Call* c, const QSize& size, bool displayPresence)
{
   if (c->peerContactMethod()->contact()) {
      return contactPhoto(c->peerContactMethod()->contact(),size,displayPresence);
   }
   else
      return QPixmap(callStateIcons[c->state()]);
}

QVariant KDEPixmapManipulation::numberCategoryIcon(const QVariant& p, const QSize& size, bool displayPresence, bool isPresent)
{
   Q_UNUSED(size);
   if (displayPresence) {
      QPixmap pxm;
      if(p.isValid())
         pxm = qvariant_cast<QPixmap>(p);
      else
         pxm = QPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/mini/call.png"));
      QPainter painter(&pxm);
      painter.setOpacity(0.3);
      painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
      painter.fillRect(pxm.rect(),isPresent?Qt::green:Qt::red);
      return pxm;
   }
   if (p.isValid())
      return qvariant_cast<QPixmap>(p);
   return QPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/mini/call.png"));
}

QVariant KDEPixmapManipulation::securityIssueIcon(const QModelIndex& index)
{
   SecurityEvaluationModel::Severity sev = qvariant_cast<SecurityEvaluationModel::Severity>(index.data((int)SecurityEvaluationModel::Role::Severity));
   switch(sev) {
      case SecurityEvaluationModel::Severity::INFORMATION:
         return QIcon::fromTheme("dialog-information");
      case SecurityEvaluationModel::Severity::WARNING:
         return QIcon::fromTheme("dialog-warning");
      case SecurityEvaluationModel::Severity::ISSUE:
      case SecurityEvaluationModel::Severity::FATAL_WARNING:
         return QIcon::fromTheme("view-barcode");
      case SecurityEvaluationModel::Severity::ERROR:
         return QIcon::fromTheme("dialog-error");
      case SecurityEvaluationModel::Severity::UNSUPPORTED:
      case SecurityEvaluationModel::Severity::COUNT__:
         break;
   }
   return QVariant();
}

QByteArray KDEPixmapManipulation::toByteArray(const QVariant& pxm)
{
   //Preparation of our QPixmap
   QByteArray bArray;
   QBuffer buffer(&bArray);
   buffer.open(QIODevice::WriteOnly);

   //PNG ?
   (qvariant_cast<QPixmap>(pxm)).save(&buffer, "PNG");
   buffer.close();

   return bArray;
}

QVariant KDEPixmapManipulation::personPhoto(const QByteArray& data, const QString& type)
{
   QImage image;
   //For now, ENCODING is only base64 and image type PNG or JPG
   const bool ret = image.loadFromData(QByteArray::fromBase64(data),type.toLatin1());
   if (!ret)
      qDebug() << "vCard image loading failed";

   return QPixmap::fromImage(image);
}

QPixmap KDEPixmapManipulation::drawDefaultUserPixmap(const QSize& size, bool displayPresence, bool isPresent) {
   //Load KDE default user pixmap
   QPixmap pxm(size);
   pxm.fill(Qt::transparent);
   QPainter painter(&pxm);

   painter.drawPixmap(3,3,QIcon::fromTheme("user-identity").pixmap(QSize(size.height()-6,size.width()-6)));

   //Create a region where the pixmap is not fully transparent
   if (displayPresence) {
      static QHash<int,QRegion> r,ri;
      static QHash<int,bool> init;
      const int width = size.width();
      if (!init[width]) {
         r[width] = QRegion(QPixmap::fromImage(pxm.toImage().createAlphaMask()));
         ri[width] = r[width].xored(pxm.rect());
         init[width] = true;
      }

      static QColor presentBrush = KStatefulBrush( KColorScheme::Window, KColorScheme::PositiveText ).brush(QPalette::Normal).color();
      static QColor awayBrush    = KStatefulBrush( KColorScheme::Window, KColorScheme::NegativeText ).brush(QPalette::Normal).color();


      painter.setOpacity(0.05);
      painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
      painter.fillRect(pxm.rect(),isPresent?Qt::green:Qt::red);
      painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

      //Paint only outside of the pixmap, it looks better
      painter.setClipRegion(ri[width]);
      QPainterPath p;
      p.addRegion(r[width]);
      QPen pen = painter.pen();
      pen.setColor(isPresent?presentBrush:awayBrush);
      painter.setBrush(Qt::NoBrush);
      painter.setRenderHint  (QPainter::Antialiasing, true      );
      for (int i=2;i<=9;i+=2) {
         pen.setWidth(i);
         painter.setPen(pen);
         painter.setOpacity(0.30f-(((i-2)*0.45f)/10.0f));
         painter.drawPath(p);
      }
   }
   return pxm;
}

QVariant KDEPixmapManipulation::userActionIcon(const UserActionElement& state) const
{

   switch(state.action) {
      case UserActionModel::Action::ACCEPT          :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/accept.svg"   ));
      case UserActionModel::Action::HOLD            :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/hold.svg"     ));
      case UserActionModel::Action::MUTE_AUDIO      :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/mic.svg"  ));
      case UserActionModel::Action::MUTE_VIDEO      :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/mic.svg"  ));
      case UserActionModel::Action::SERVER_TRANSFER :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/transfert.svg" ));
      case UserActionModel::Action::RECORD          :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/rec_call.svg"   ));
      case UserActionModel::Action::HANGUP          :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/refuse.svg"   ));
      case UserActionModel::Action::JOIN            :
         return QIcon();
      case UserActionModel::Action::ADD_NEW         :
         return QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ring-kde/light/accept.svg"   ));
      case UserActionModel::Action::COUNT__:
         break;
   };

   return QVariant();
}

QVariant KDEPixmapManipulation::collectionIcon(const CollectionInterface* interface, PixmapManipulationDelegate::CollectionIconHint hint) const
{
   Q_UNUSED(interface)

   switch(hint) {
      case PixmapManipulationDelegate::CollectionIconHint::CONTACT:
         return QIcon::fromTheme("folder-publicshare");
      case PixmapManipulationDelegate::CollectionIconHint::HISTORY:
         return QIcon::fromTheme("view-history");
      case PixmapManipulationDelegate::CollectionIconHint::BOOKMARK:
         return QIcon::fromTheme("folder-bookmark");
      case PixmapManipulationDelegate::CollectionIconHint::CERTIFICATE:
         return QIcon::fromTheme("certificate-server");
      case PixmapManipulationDelegate::CollectionIconHint::RINGTONE:
      case PixmapManipulationDelegate::CollectionIconHint::NONE:
      case PixmapManipulationDelegate::CollectionIconHint::PROFILE:
      case PixmapManipulationDelegate::CollectionIconHint::PHONE_NUMBER:
      case PixmapManipulationDelegate::CollectionIconHint::ACCOUNT:
         break;
   };

   return QVariant();
}

const char* KDEPixmapManipulation::icnPath[2][2] = {
   {
   /* INCOMING  */ RingIcons::HISTORY_INCOMING  ,
   /* OUTGOING  */ RingIcons::HISTORY_OUTGOING  ,
   },
   {
   /* MISSED_IN */ RingIcons::HISTORY_MISSED    ,
   /* MISSED_OUT*/ RingIcons::HISTORY_MISSED_OUT,
   }
};

QVariant KDEPixmapManipulation::securityLevelIcon(const SecurityEvaluationModel::SecurityLevel level) const
{

   QPixmap pxm(20,20);
   QPainter p(&pxm);

   static QStringList colors  = {
      "#A02111",
      "#A05C0F",
      "#CBC910",
      "#6DA00F",
      "#0EA02B",
      "#A02111",
      "#A02111"
   };

   const QColor col = colors[(int)level];
   p.setBrush(col);
   p.drawRect(0,0,20,20);

   return pxm;
}

