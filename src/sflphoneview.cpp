/***************************************************************************
 *   Copyright (C) 2009-2013 by Savoir-Faire Linux                         *
 *   Author : Jérémy Quentin <jeremy.quentin@savoirfairelinux.com>         *
 *            Emmanuel Lepage Vallee <emmanuel.lepage@savoirfairelinux.com>*
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

//Parent
#include "sflphoneview.h"

//Qt
#include <QtCore/QString>
#include <QtCore/QPointer>
#include <QtGui/QPalette>
#include <QtGui/QWidget>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtCore/QAbstractAnimation>

//KDE
#include <KLocale>
#include <KAction>
#include <KMenu>
#include <KInputDialog>
#include <kabc/addressbook.h>
#include <KMessageBox>
#include <KIcon>
#include <KColorScheme>
#include <KStandardDirs>

//sflphone
#include "accountwizard.h"
#include "sflphone.h"
#include "canvasobjectmanager.h"
#include "widgets/tips/tipcollection.h"
#include "widgets/callviewtoolbar.h"
#include "extendedaction.h"
#include "eventmanager.h"
#include "actioncollection.h"
#include "delegates/conferencedelegate.h"
#include "delegates/historydelegate.h"
#include "delegates/categorizeddelegate.h"
#include "widgets/tips/dialpadtip.h"
#include "widgets/kphonenumberselector.h"
#include "widgets/callviewoverlay.h"
#include "widgets/autocompletion.h"

//sflphone library
#include "klib/kcfg_settings.h"
#include "lib/phonenumber.h"
#include "lib/typedefs.h"
#include "lib/dbus/configurationmanager.h"
#include "lib/dbus/callmanager.h"
#include "lib/dbus/instancemanager.h"
#include "lib/sflphone_const.h"
#include "lib/contact.h"
#include "lib/accountlistmodel.h"
#include "lib/phonedirectorymodel.h"
#include "lib/audiosettingsmodel.h"
#include "klib/helperfunctions.h"
#include "klib/tipmanager.h"
#include "lib/visitors/accountlistcolorvisitor.cpp"
#include "lib/visitors/pixmapmanipulationvisitor.cpp"

#define IM_ACTIVE m_pMessageTabBox->isVisible()

QDebug operator<<(QDebug dbg, const Call::State& c)
{
   dbg.nospace() << QString(Call::toHumanStateName(c));
   return dbg.space();
}

QDebug operator<<(QDebug dbg, const Call::DaemonState& c)
{
   dbg.nospace() << static_cast<int>(c);
   return dbg.space();
}

QDebug operator<<(QDebug dbg, const Call::Action& c)
{
   dbg.nospace() << static_cast<int>(c);
   return dbg.space();
}

class ColorVisitor : public AccountListColorVisitor {
public:
   ColorVisitor(QPalette pal) : m_Pal(pal) {
      m_Green = QColor(m_Pal.color(QPalette::Base));
      if (m_Green.green()+20 >= 255) {
         m_Green.setRed ( ((int)m_Green.red()  -20));
         m_Green.setBlue( ((int)m_Green.blue() -20));
      }
      else
         m_Green.setGreen(((int)m_Green.green()+20));

      m_Red = QColor(m_Pal.color(QPalette::Base));
      if (m_Red.red()+20 >= 255) {
         m_Red.setGreen(  ((int)m_Red.green()  -20));
         m_Red.setBlue(   ((int)m_Red.blue()   -20));
      }
      else
         m_Red.setRed(    ((int)m_Red.red()     +20));

      m_Yellow = QColor(m_Pal.color(QPalette::Base));
      if (m_Yellow.red()+20 >= 255 || m_Green.green()+20 >= 255) {
         m_Yellow.setBlue(((int)m_Yellow.blue() -20));
      }
      else {
         m_Yellow.setGreen(((int)m_Yellow.green()+20));
         m_Yellow.setRed( ((int)m_Yellow.red()   +20));
      }
   }

   virtual QVariant getColor(const Account* a) {
      if(a->registrationStatus() == Account::State::UNREGISTERED || !a->isEnabled())
         return m_Pal.color(QPalette::Base);
      if(a->registrationStatus() == Account::State::REGISTERED || a->registrationStatus() == Account::State::READY) {
         return m_Green;
      }
      if(a->registrationStatus() == Account::State::TRYING)
         return m_Yellow;
      return m_Red;
   }

   virtual QVariant getIcon(const Account* a) {
      if (a->state() == Account::AccountEditState::MODIFIED)
         return KIcon("document-save");
      else if (a->state() == Account::AccountEditState::OUTDATED) {
         return KIcon("view-refresh");
      }
      return QVariant();
   }
private:
   QPalette m_Pal;
   QColor   m_Green;
   QColor   m_Yellow;
   QColor   m_Red;
};

class KDEPixmapManipulation : public PixmapManipulationVisitor {
public:
   KDEPixmapManipulation() : PixmapManipulationVisitor() {}
   QVariant contactPhoto(Contact* c, QSize size, bool displayPresence = true) {
      QVariant preRendered = c->property(QString("photo2"+QString::number(size.height())).toAscii());
      if (preRendered.isValid())
         return preRendered;
      const int radius = (size.height() > 35) ? 7 : 5;
      const QPixmap* pxmPtr = c->photo();
      bool isTracked = displayPresence && c->isTracked();
      bool isPresent = displayPresence && c->isPresent();
      static QColor presentBrush = KStatefulBrush( KColorScheme::Window, KColorScheme::PositiveText ).brush(QPalette::Normal).color();
      static QColor awayBrush    = KStatefulBrush( KColorScheme::Window, KColorScheme::NegativeText ).brush(QPalette::Normal).color();

      QPixmap pxm;
      if (pxmPtr) {
         QPixmap contactPhoto(pxmPtr->scaledToWidth(size.height()-6));
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
         QPen pen(QApplication::palette().color(QPalette::Disabled,QPalette::Text));
         pen.setWidth(isTracked?1:2);
         painter.setPen(pen);
         painter.setRenderHint  (QPainter::Antialiasing, true   );
         painter.drawRoundedRect(3,3,pxm.height()-6,pxm.height()-6,radius,radius);

         //Draw the glow around pixmaps
         if (isTracked) {
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

      c->setProperty(QString("photo2"+QString::number(size.height())).toAscii(),pxm);
      return pxm;
   }

   virtual QVariant callPhoto(const PhoneNumber* n, QSize size, bool displayPresence = true) {
      if (n->contact()) {
         return contactPhoto(n->contact(),size,displayPresence);
      }
      else {
         bool isTracked = displayPresence && n->isTracked();
         bool isPresent = displayPresence && n->isPresent();

         return drawDefaultUserPixmap(size,isTracked,isPresent);
      }
   }

   virtual QVariant callPhoto(Call* c, QSize size, bool displayPresence = true) {
      if (c->peerPhoneNumber()->contact()) {
         return contactPhoto(c->peerPhoneNumber()->contact(),size,displayPresence);
      }
      else
         return QPixmap(callStateIcons[c->state()]);
   }

   QVariant numberCategoryIcon(const QPixmap* p, QSize size, bool displayPresence = false, bool isPresent = false) {
      Q_UNUSED(size)
      if (displayPresence) {
         QPixmap pxm = p?(*p):QPixmap(KStandardDirs::locate("data" , "sflphone-client-kde/mini/call.png"));
         QPainter painter(&pxm);
         painter.setOpacity(0.3);
         painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
         painter.fillRect(pxm.rect(),isPresent?Qt::green:Qt::red);
         return pxm;
      }
      if (p)
         return *p;
      return QPixmap(KStandardDirs::locate("data" , "sflphone-client-kde/mini/call.png"));
   }

private:
   static const TypedStateMachine< const char* , Call::State > callStateIcons;

   static const char* icnPath[4];

   //Helper
   QPixmap drawDefaultUserPixmap(QSize size, bool displayPresence, bool isPresent) {
      //Load KDE default user pixmap
      QPixmap pxm(size);
      pxm.fill(Qt::transparent);
      QPainter painter(&pxm);

      painter.drawPixmap(3,3,KIcon("user-identity").pixmap(QSize(size.height()-6,size.width()-6)));

      //Create a region where the pixmap is not fully transparent
      if (displayPresence) {
         static QRegion r,ri;
         static bool init = false;
         if (!init) {
            r = QRegion(QPixmap::fromImage(pxm.toImage().createAlphaMask()));
            ri = r.xored(pxm.rect());
            init = true;
         }

         static QColor presentBrush = KStatefulBrush( KColorScheme::Window, KColorScheme::PositiveText ).brush(QPalette::Normal).color();
         static QColor awayBrush    = KStatefulBrush( KColorScheme::Window, KColorScheme::NegativeText ).brush(QPalette::Normal).color();


         painter.setOpacity(0.05);
         painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
         painter.fillRect(pxm.rect(),isPresent?Qt::green:Qt::red);
         painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

         //Paint only outside of the pixmap, it looks better
         painter.setClipRegion(ri);
         QPainterPath p;
         p.addRegion(r);
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
};
const TypedStateMachine< const char* , Call::State > KDEPixmapManipulation::callStateIcons = {
   {  ICON_INCOMING   ,
      ICON_RINGING    ,
      ICON_CURRENT    ,
      ICON_DIALING    ,
      ICON_HOLD       ,
      ICON_FAILURE    ,
      ICON_BUSY       ,
      ICON_TRANSFER   ,
      ICON_TRANSF_HOLD,
      ""              ,
      ""              ,
      ICON_CONFERENCE}};

const char* KDEPixmapManipulation::icnPath[4] = {
   /* INCOMING */ ICON_HISTORY_INCOMING,
   /* OUTGOING */ ICON_HISTORY_OUTGOING,
   /* MISSED   */ ICON_HISTORY_MISSED  ,
   /* NONE     */ ""                   ,
};

///Constructor
SFLPhoneView::SFLPhoneView(QWidget *parent)
   : QWidget(parent),m_pTransferOverlay(nullptr),m_pAutoCompletion(nullptr)
{
   new KDEPixmapManipulation();
   setupUi(this);
   KPhoneNumberSelector::init();

   //Enable on-canvas messages
   TipCollection::setManager(new TipManager(m_pView));
   m_pCanvasManager = new CanvasObjectManager();

   //Set global settings
   AudioSettingsModel::instance()->setEnableRoomTone(ConfigurationSkeleton::enableRoomTone());

   m_pEventManager = new EventManager(this);
   m_pView->setModel(CallModel::instance());
   TipCollection::manager()->changeSize();
   connect(CallModel::instance(),SIGNAL(layoutChanged()),m_pView,SLOT(expandAll()));
   m_pView->expandAll();
   m_pConfDelegate = new ConferenceDelegate(m_pView,palette());
   m_pHistoryDelegate = new HistoryDelegate(m_pView);
   m_pConfDelegate->setCallDelegate(m_pHistoryDelegate);
   m_pView->setItemDelegate(m_pConfDelegate);
   m_pView->viewport()->installEventFilter(m_pEventManager);
   m_pView->installEventFilter(m_pEventManager);
   m_pView->setViewType(CategorizedTreeView::ViewType::Call);


   if (!CallModel::instance()->getCallList().size())
      m_pCanvasManager->newEvent(CanvasObjectManager::CanvasEvent::NO_CALLS);

   QPalette pal = QPalette(palette());
   pal.setColor(QPalette::AlternateBase, Qt::lightGray);
   setPalette(pal);

   AccountListModel::instance()->setColorVisitor(new ColorVisitor(pal));

   m_pMessageBoxW->setVisible(false);

   //                SENDER                             SIGNAL                              RECEIVER                SLOT                      /
   /**/connect(CallModel::instance()        , SIGNAL(incomingCall(Call*))                   , this   , SLOT(on1_incomingCall(Call*))          );
   /**/connect(AccountListModel::instance() , SIGNAL(voiceMailNotify(Account*,int))         , this   , SLOT(on1_voiceMailNotify(Account*,int)) );
   /**/connect(CallModel::instance()        , SIGNAL(callStateChanged(Call*,Call::State))   , this   , SLOT(updateWindowCallState())          );
   /**/connect(AccountListModel::instance() , SIGNAL(accountListUpdated())                  , this   , SLOT(updateWindowCallState())          );
   /**/connect(m_pSendMessageLE             , SIGNAL(returnPressed())                       , this   , SLOT(sendMessage())                    );
   /**/connect(m_pSendMessagePB             , SIGNAL(clicked())                             , this   , SLOT(sendMessage())                    );
   /**/connect(m_pView                      , SIGNAL(itemDoubleClicked(QModelIndex)), m_pEventManager, SLOT(enter())                          );
   /*                                                                                                                                        */

   //Auto completion
   loadAutoCompletion();

   m_pCanvasToolbar = new CallViewToolbar(m_pView);
   connect(m_pView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)) , m_pCanvasToolbar, SLOT(updateState()));
   connect(CallModel::instance()    , SIGNAL(callStateChanged(Call*,Call::State))     , m_pCanvasToolbar, SLOT(updateState()));
   connect(CallModel::instance()    , SIGNAL(layoutChanged())                         , m_pCanvasToolbar, SLOT(updateState()));
}

///Destructor
SFLPhoneView::~SFLPhoneView()
{
   m_pView->setItemDelegate(nullptr);
   delete m_pConfDelegate;
   delete m_pHistoryDelegate;
   if (m_pAutoCompletion)
      delete m_pAutoCompletion;
}

///Init main window
void SFLPhoneView::loadWindow()
{
   updateWindowCallState ();
   updateRecordButton    ();
   updateVolumeButton    ();
   updateRecordBar       ();
   updateVolumeBar       ();
   updateVolumeControls  ();
   loadAutoCompletion    ();
   widget_dialpad->setVisible(ConfigurationSkeleton::displayDialpad());
   AudioSettingsModel::instance()->setEnableRoomTone(ConfigurationSkeleton::enableRoomTone());
}


/*****************************************************************************
 *                                                                           *
 *                                  Setters                                  *
 *                                                                           *
 ****************************************************************************/

///Set the current selection item
void SFLPhoneView::setCurrentIndex(const QModelIndex& idx)
{
   m_pView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent);
}

/*****************************************************************************
 *                                                                           *
 *                                  Getters                                  *
 *                                                                           *
 ****************************************************************************/

///Return the auto completion widget
AutoCompletion* SFLPhoneView::autoCompletion() const
{
   return m_pAutoCompletion;
}

///Return the current (selected) call
Call* SFLPhoneView::currentCall() const
{
   return CallModel::instance()->getCall(m_pView->selectionModel()->currentIndex());
}

/*****************************************************************************
 *                                                                           *
 *                              Keyboard input                               *
 *                                                                           *
 ****************************************************************************/

///Create a call from the clipboard content
void SFLPhoneView::paste()
{
   QClipboard* cb = QApplication::clipboard();
   const QMimeData* data = cb->mimeData();
   if (data->hasFormat(MIME_PHONENUMBER)) {
      m_pEventManager->typeString(data->data(MIME_PHONENUMBER));
   }
   else {
      m_pEventManager->typeString(cb->text());
   }
}


/*****************************************************************************
 *                                                                           *
 *                                  Mutator                                  *
 *                                                                           *
 ****************************************************************************/

///Select a phone number when calling using a contact
bool SFLPhoneView::selectCallPhoneNumber(Call** call2,Contact* contact)
{
   if (contact->phoneNumbers().count() == 1) {
      *call2 = CallModel::instance()->addDialingCall(contact->formattedName(),AccountListModel::currentAccount());
      if (*call2)
         (*call2)->appendText(contact->phoneNumbers()[0]->uri());
   }
   else if (contact->phoneNumbers().count() > 1) {
      const PhoneNumber* number = KPhoneNumberSelector().getNumber(contact);
      if (!number->uri().isEmpty()) {
         (*call2) = CallModel::instance()->addDialingCall(contact->formattedName(), AccountListModel::currentAccount());
         if (*call2)
            (*call2)->appendText(number->uri());
      }
      else {
         kDebug() << "Operation cancelled";
         return false;
      }
   }
   else {
      kDebug() << "This contact have no valid phone number";
      return false;
   }
   return true;
} //selectCallPhoneNumber


/*****************************************************************************
 *                                                                           *
 *                       Update display related code                         *
 *                                                                           *
 ****************************************************************************/

///Change GUI icons
void SFLPhoneView::updateWindowCallState()
{
   kDebug() << "Call state changed";
   bool    enabledActions [6] = { true             , true                , true             , true                 , true               , true                 };
   QString buttonIconFiles[6] = { ICON_CALL        , ICON_HANGUP         , ICON_HOLD        , ICON_TRANSFER        , ICON_REC_DEL_OFF   , ICON_MAILBOX         };
   QString actionTexts    [6] = { ACTION_LABEL_CALL, ACTION_LABEL_HANG_UP, ACTION_LABEL_HOLD, ACTION_LABEL_TRANSFER, ACTION_LABEL_RECORD, ACTION_LABEL_MAILBOX };

   Call* call = 0;

   bool transfer(false),recordActivated(false);

   enabledActions[SFLPhone::CallAction::Mailbox] = AccountListModel::currentAccount() && ! AccountListModel::currentAccount()->mailbox().isEmpty();

   call = CallModel::instance()->getCall(m_pView->selectionModel()->currentIndex());
   if (!call) {
      kDebug() << "No item selected.";
      enabledActions[ SFLPhone::CallAction::Refuse   ] = false;
      enabledActions[ SFLPhone::CallAction::Hold     ] = false;
      enabledActions[ SFLPhone::CallAction::Transfer ] = false;
      enabledActions[ SFLPhone::CallAction::Record   ] = false;
      m_pMessageBoxW->setVisible(false);
   }
   else if (call->isConference()) {
      //TODO Something to do?
   }
   else {
      Call::State state = call->state();
      recordActivated = call->isRecording();

      switch (state) {
         case Call::State::INCOMING:
            buttonIconFiles [ SFLPhone::CallAction::Accept   ] = ICON_ACCEPT                 ;
            buttonIconFiles [ SFLPhone::CallAction::Refuse   ] = ICON_REFUSE                 ;
            actionTexts     [ SFLPhone::CallAction::Accept   ] = ACTION_LABEL_ACCEPT         ;
            actionTexts     [ SFLPhone::CallAction::Refuse   ] = ACTION_LABEL_REFUSE         ;
            m_pMessageBoxW->setVisible(false || IM_ACTIVE)   ;
            break;

         case Call::State::RINGING:
            enabledActions  [ SFLPhone::CallAction::Hold     ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Transfer ] = false                       ;
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::CURRENT:
            buttonIconFiles [ SFLPhone::CallAction::Record   ] = ICON_REC_DEL_ON             ;
            m_pMessageBoxW->setVisible((true && ConfigurationSkeleton::displayMessageBox()) || IM_ACTIVE);
            break;

         case Call::State::DIALING:
            enabledActions  [ SFLPhone::CallAction::Hold     ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Transfer ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Record   ] = false                       ;
            actionTexts     [ SFLPhone::CallAction::Accept   ] = ACTION_LABEL_ACCEPT         ;
            buttonIconFiles [ SFLPhone::CallAction::Accept   ] = ICON_ACCEPT                 ;
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::HOLD:
            buttonIconFiles [ SFLPhone::CallAction::Hold     ] = ICON_UNHOLD                 ;
            actionTexts     [ SFLPhone::CallAction::Hold     ] = ACTION_LABEL_UNHOLD         ;
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::FAILURE:
            //enabledActions  [ SFLPhone::Accept   ] = false                     ;
            enabledActions  [ SFLPhone::CallAction::Hold     ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Transfer ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Record   ] = false                       ;
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::BUSY:
            //enabledActions  [ SFLPhone::Accept   ] = false                     ;
            enabledActions  [ SFLPhone::CallAction::Hold     ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Transfer ] = false                       ;
            enabledActions  [ SFLPhone::CallAction::Record   ] = false                       ;
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::TRANSFERRED:
            buttonIconFiles [ SFLPhone::CallAction::Accept   ] = ICON_EXEC_TRANSF            ;
            actionTexts     [ SFLPhone::CallAction::Transfer ] = ACTION_LABEL_GIVE_UP_TRANSF ;
            buttonIconFiles [ SFLPhone::CallAction::Record   ] = ICON_REC_DEL_ON             ;
            m_pMessageBoxW->setVisible(false || IM_ACTIVE)                       ;
            if (!m_pTransferOverlay) {
               m_pTransferOverlay = new CallViewOverlay(m_pView);
            }
            m_pTransferOverlay->setCurrentCall(call);
            m_pTransferOverlay->setVisible(true);
            transfer = true;
            break;

         case Call::State::TRANSF_HOLD:
            buttonIconFiles [ SFLPhone::CallAction::Accept   ] = ICON_EXEC_TRANSF            ;
            buttonIconFiles [ SFLPhone::CallAction::Hold     ] = ICON_UNHOLD                 ;
            actionTexts     [ SFLPhone::CallAction::Transfer ] = ACTION_LABEL_GIVE_UP_TRANSF ;
            actionTexts     [ SFLPhone::CallAction::Hold     ] = ACTION_LABEL_UNHOLD         ;
            m_pMessageBoxW->setVisible(false)                                    ;
            transfer = true;
            break;

         case Call::State::OVER:
            kDebug() << "Error : Reached CALL_STATE_OVER with call "  << call->id() << "!";
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::ERROR:
            kDebug() << "Error : Reached CALL_STATE_ERROR with call " << call->id() << "!";
            m_pMessageBoxW->setVisible(false)                                    ;
            break;

         case Call::State::CONFERENCE:
            enabledActions  [ SFLPhone::CallAction::Transfer ] = false                       ;
            m_pMessageBoxW->setVisible(false || IM_ACTIVE)                       ;
            break;

         case Call::State::CONFERENCE_HOLD:
            enabledActions  [ SFLPhone::CallAction::Transfer ] = false                       ;
            m_pMessageBoxW->setVisible(false)                                    ;
            break;
         case Call::State::COUNT:
         default:
            kDebug() << "Error : Reached unexisting state for call "  << call->id() << "(" << call->state() << "!";
            break;

      }

      if (TipCollection::dragAndDrop()) {
         int activeCallCounter=0;
         foreach (Call* call2, CallModel::instance()->getCallList()) {
            if (dynamic_cast<Call*>(call2)) {
               activeCallCounter += (call2->state() == Call::State::CURRENT || call2->state() == Call::State::HOLD);
               activeCallCounter -= (call2->state() == Call::State::INCOMING || call2->state() ==Call::State::RINGING)*1000;
            }
         }
         if (activeCallCounter >= 2 && !CallModel::instance()->getConferenceList().size()) {
            m_pCanvasManager->newEvent(CanvasObjectManager::CanvasEvent::CALL_COUNT_CHANGED);
         }
      }
   }

   emit enabledActionsChangeAsked     ( enabledActions  );
   emit actionIconsChangeAsked        ( buttonIconFiles );
   emit actionTextsChangeAsked        ( actionTexts     );
   emit transferCheckStateChangeAsked ( transfer        );
   emit recordCheckStateChangeAsked   ( recordActivated );

} //updateWindowCallState

void SFLPhoneView::loadAutoCompletion()
{
   if (ConfigurationSkeleton::enableAutoCompletion()) {
      if (!m_pAutoCompletion) {
         m_pAutoCompletion = new AutoCompletion(m_pView);
         PhoneDirectoryModel::instance()->setCallWithAccount(ConfigurationSkeleton::autoCompleteUseAccount());
         m_pAutoCompletion->setUseUnregisteredAccounts(ConfigurationSkeleton::autoCompleteMergeNumbers());
         connect(m_pAutoCompletion, SIGNAL(requestVisibility(bool)), m_pEventManager, SLOT(slotAutoCompletionVisibility(bool)));
         connect(m_pAutoCompletion,SIGNAL(doubleClicked(PhoneNumber*)),this,SLOT(slotAutoCompleteClicked(PhoneNumber*)));
      }
   }
   else if (m_pAutoCompletion) {
      delete m_pAutoCompletion;
      m_pAutoCompletion = nullptr;
   }
}

//Mute a call
void SFLPhoneView::mute(bool value)
{
   CallManagerInterface & callManager = DBus::CallManager::instance();
   kDebug() << "on_toolButton_recVol_clicked().";
   if(!value) {
      toolButton_recVol->setChecked(false);
      slider_recVol->setEnabled(true);
      Q_NOREPLY callManager.setVolume(RECORD_DEVICE, (double)slider_recVol->value() / 100.0);
      ActionCollection::instance()->muteAction()->setText(i18nc("Mute the current audio device","Mute"));
   }
   else {
      toolButton_recVol->setChecked(true);
      slider_recVol->setEnabled(false);
      Q_NOREPLY callManager.setVolume(RECORD_DEVICE, 0.0);
      ActionCollection::instance()->muteAction()->setText(i18nc("Unmute the current audio device","Unmute"));
   }
   updateRecordButton();
}

///Change icon of the record button
void SFLPhoneView::updateRecordButton()
{
   kDebug() << "updateRecordButton";
   CallManagerInterface & callManager = DBus::CallManager::instance();
   double recVol = callManager.getVolume(RECORD_DEVICE);
   if(recVol     == 0.00) {
      static const QIcon recVol0 = QIcon(ICON_REC_VOL_0);
      toolButton_recVol->setIcon(recVol0);
   }
   else if(recVol < 0.33) {
      static const QIcon recVol1 = QIcon(ICON_REC_VOL_1);
      toolButton_recVol->setIcon(recVol1);
   }
   else if(recVol < 0.67) {
      static const QIcon recVol2 = QIcon(ICON_REC_VOL_2);
      toolButton_recVol->setIcon(recVol2);
   }
   else {
      static const QIcon recVol3 = QIcon(ICON_REC_VOL_3);
      toolButton_recVol->setIcon(recVol3);
   }

   if(recVol > 0) {
      toolButton_recVol->setChecked(false);
   }
}

///Update the colunm button icon
void SFLPhoneView::updateVolumeButton()
{
   kDebug() << "updateVolumeButton";
   CallManagerInterface& callManager = DBus::CallManager::instance();
   const double sndVol = callManager.getVolume(SOUND_DEVICE);

   if(sndVol     == 0.00) {
      toolButton_sndVol->setIcon(QIcon(ICON_SND_VOL_0));
   }
   else if(sndVol < 0.33) {
      toolButton_sndVol->setIcon(QIcon(ICON_SND_VOL_1));
   }
   else if(sndVol < 0.67) {
      toolButton_sndVol->setIcon(QIcon(ICON_SND_VOL_2));
   }
   else {
      toolButton_sndVol->setIcon(QIcon(ICON_SND_VOL_3));
   }

   if(sndVol > 0) {
      toolButton_sndVol->setChecked(false);
   }
}

///Update the record bar
void SFLPhoneView::updateRecordBar(double _value)
{
   CallManagerInterface & callManager = DBus::CallManager::instance();
   const double recVol = callManager.getVolume(RECORD_DEVICE);
   kDebug() << "updateRecordBar" << recVol;
   const int value = (_value > 0)?_value:(int)(recVol * 100);
   slider_recVol->setValue(value);
}

///Update the volume bar
void SFLPhoneView::updateVolumeBar(double _value)
{
   CallManagerInterface & callManager = DBus::CallManager::instance();
   const double sndVol = callManager.getVolume(SOUND_DEVICE);
   kDebug() << "updateVolumeBar" << sndVol;
   const int value = (_value > 0)?_value:(int)(sndVol * 100);
   slider_sndVol->setValue(value);
}

///Hide or show the volume control
void SFLPhoneView::updateVolumeControls()
{
   toolButton_recVol->setVisible ( ActionCollection::instance()->displayVolumeControlsAction()->isChecked() && ConfigurationSkeleton::displayVolume() );
   toolButton_sndVol->setVisible ( ActionCollection::instance()->displayVolumeControlsAction()->isChecked() && ConfigurationSkeleton::displayVolume() );
   slider_recVol->setVisible     ( ActionCollection::instance()->displayVolumeControlsAction()->isChecked() && ConfigurationSkeleton::displayVolume() );
   slider_sndVol->setVisible     ( ActionCollection::instance()->displayVolumeControlsAction()->isChecked() && ConfigurationSkeleton::displayVolume() );
}


/*****************************************************************************
 *                                                                           *
 *                                    Slots                                  *
 *                                                                           *
 ****************************************************************************/

///Proxy to hide or show the volume control
void SFLPhoneView::displayVolumeControls(bool checked)
{
   //ConfigurationManagerInterface & configurationManager = DBus::ConfigurationManager::instance();
   ConfigurationSkeleton::setDisplayVolume(checked);
   updateVolumeControls();
}

///Proxy to hide or show the dialpad
void SFLPhoneView::displayDialpad(bool checked)
{
   ConfigurationSkeleton::setDisplayDialpad(checked);
   widget_dialpad->setVisible(ConfigurationSkeleton::displayDialpad());
}

///Display a notification popup (freedesktop notification)
void SFLPhoneView::displayMessageBox(bool checked)
{
   ConfigurationSkeleton::setDisplayMessageBox(checked);
   Call* call = CallModel::instance()->getCall(m_pView->selectionModel()->currentIndex());
   m_pMessageBoxW->setVisible(checked
      && call
      && (call->state()   == Call::State::CURRENT
         || call->state() == Call::State::HOLD
      )
   );
}

///Input grabber
void SFLPhoneView::on_widget_dialpad_typed(QString text)
{
   m_pEventManager->typeString(text);
}

///The value on the slider changed
void SFLPhoneView::on_slider_recVol_valueChanged(int value)
{
   kDebug() << "on_slider_recVol_valueChanged(" << value << ")";
   CallManagerInterface & callManager = DBus::CallManager::instance();
   Q_NOREPLY callManager.setVolume(RECORD_DEVICE, (double)value / 100.0);
   updateRecordButton();
}

///The value on the slider changed
void SFLPhoneView::on_slider_sndVol_valueChanged(int value)
{
   kDebug() << "on_slider_sndVol_valueChanged(" << value << ")";
   CallManagerInterface & callManager = DBus::CallManager::instance();
   Q_NOREPLY callManager.setVolume(SOUND_DEVICE, (double)value / 100.0);
   updateVolumeButton();
}

///The mute button have been clicked
void SFLPhoneView::on_toolButton_recVol_clicked(bool checked)
{
   mute(checked);
}

///The mute button have been clicked
void SFLPhoneView::on_toolButton_sndVol_clicked(bool checked)
{
   CallManagerInterface & callManager = DBus::CallManager::instance();
   kDebug() << "on_toolButton_sndVol_clicked().";
   if(!checked) {
      toolButton_sndVol->setChecked(false);
      slider_sndVol->setEnabled(true);
      Q_NOREPLY callManager.setVolume(SOUND_DEVICE, (double)slider_sndVol->value() / 100.0);
   }
   else {
      toolButton_sndVol->setChecked(true);
      slider_sndVol->setEnabled(false);
      Q_NOREPLY callManager.setVolume(SOUND_DEVICE, 0.0);
   }

   updateVolumeButton();
}

///When a call is coming (dbus)
void SFLPhoneView::on1_incomingCall(Call* call)
{
   kDebug() << "Signal : Incoming Call ! ID = " << call->id();

   updateWindowCallState();

   if (ConfigurationSkeleton::displayOnCalls()) {
      SFLPhone::app()->activateWindow(      );
      SFLPhone::app()->raise         (      );
      SFLPhone::app()->setVisible    ( true );
   }

   const QModelIndex& idx = CallModel::instance()->getIndex(call);
   if (idx.isValid() && (call->state() == Call::State::RINGING || call->state() == Call::State::INCOMING)) {
      m_pView->selectionModel()->clearSelection();
      m_pView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent);
   }

   emit incomingCall(call);
}

///When a new voice mail is coming
void SFLPhoneView::on1_voiceMailNotify(Account* a, int count)
{
   kDebug() << "Signal : VoiceMail Notify ! " << count << " new voice mails for account " << a->alias();
}

///When the volume change
void SFLPhoneView::on1_volumeChanged(const QString& device, double value)
{
   Q_UNUSED(device)
   kDebug() << "Signal : Volume Changed !" << value;
   if(! (toolButton_recVol->isChecked() && value == 0.0))
      updateRecordBar(value);
   if(! (toolButton_sndVol->isChecked() && value == 0.0))
      updateVolumeBar(value);
}

///Send a text message
void SFLPhoneView::sendMessage()
{
   Call* call = CallModel::instance()->getCall(m_pView->selectionModel()->currentIndex());
   if (dynamic_cast<Call*>(call) && !m_pSendMessageLE->text().isEmpty()) {
      call->sendTextMessage(m_pSendMessageLE->text());
   }
   m_pSendMessageLE->clear();
}

void SFLPhoneView::slotAutoCompleteClicked(PhoneNumber* n)
{
   Call* call = currentCall();
   if (call->state() == Call::State::DIALING) {
      call->setDialNumber(n);
      if (n->account())
         call->setAccount(n->account());
      call->performAction(Call::Action::ACCEPT);
      m_pAutoCompletion->reset();
   }
}

#undef IM_ACTIVE

#include "sflphoneview.moc"
