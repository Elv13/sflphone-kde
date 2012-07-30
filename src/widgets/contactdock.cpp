/***************************************************************************
 *   Copyright (C) 2009-2012 by Savoir-Faire Linux                         *
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

//Parent
#include "contactdock.h"

//Qt
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QTreeWidget>
#include <QtGui/QSpacerItem>
#include <QtGui/QHeaderView>
#include <QtGui/QCheckBox>
#include <QtGui/QSplitter>
#include <QtGui/QLabel>

//KDE
#include <KDebug>
#include <KLineEdit>
#include <KLocalizedString>
#include <KIcon>
#include <KComboBox>

//SFLPhone
#include "contactitemwidget.h"
#include "sflphone.h"
#include "callview.h"
#include "sflphoneview.h"

//SFLPhone library
#include "lib/historymodel.h"
#include "lib/call.h"
#include "lib/contact.h"
#include "klib/akonadibackend.h"
#include "klib/configurationskeleton.h"

#define CURRENT_SORTING_MODE m_pSortByCBB->currentIndex()

///QNumericTreeWidgetItem_hist: TreeWidget using different sorting criterias
class QNumericTreeWidgetItem_hist : public QTreeWidgetItem {
   public:
      QNumericTreeWidgetItem_hist(QTreeWidget* parent):QTreeWidgetItem(parent),widget(0),weight(-1){}
      QNumericTreeWidgetItem_hist(QTreeWidgetItem* parent=0):QTreeWidgetItem(parent),widget(0),weight(-1){}
      ContactItemWidget* widget;
      QString number;
      int weight;
   private:
      bool operator<(const QTreeWidgetItem & other) const {
         int column = treeWidget()->sortColumn();
         return text(column) < other.text(column);
      }
};

///A Phone number in contact with many of them (not used when 0 or 1 number)
class PhoneNumberItem : public QWidget {
public:
   PhoneNumberItem(QString number, QString type,QString name, QWidget* parent = nullptr) : QWidget(parent),m_pNumber(number),m_pType(type),m_pName(name) {
      QHBoxLayout* l   = new QHBoxLayout(this);
      QLabel* numberL  = new QLabel(" <b>"+type+":</b>",this);
      QLabel* number2L = new QLabel(number,this);
      l->addWidget(numberL);
      l->addWidget(number2L);
      l->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
      numberL->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
   }
protected:
   virtual void mouseDoubleClickEvent(QMouseEvent *e )
   {
      e->accept();
      Call* call = SFLPhone::model()->addDialingCall(m_pName, AccountList::getCurrentAccount());
      if (call) {
         call->setCallNumber(m_pNumber);
         call->setPeerName(m_pName);
         call->actionPerformed(CALL_ACTION_ACCEPT);
      }
      else {
         HelperFunctions::displayNoAccountMessageBox(this);
      }
   }
private:
   QString m_pNumber;
   QString m_pType;
   QString m_pName;
};


///Forward keypresses to the filter line edit
bool KeyPressEaterC::eventFilter(QObject *obj, QEvent *event)
{
   if (event->type() == QEvent::KeyPress) {
      m_pDock->keyPressEvent((QKeyEvent*)event);
      return true;
   } else {
      // standard event processing
      return QObject::eventFilter(obj, event);
   }
} //eventFilter

///Constructor
ContactDock::ContactDock(QWidget* parent) : QDockWidget(parent)
{
   setObjectName("contactDock");
   m_pFilterLE     = new KLineEdit   (                   );
   m_pSplitter     = new QSplitter   ( Qt::Vertical,this );
   m_pSortByCBB    = new KComboBox   ( this              );
   m_pContactView  = new ContactTree ( this              );
   m_pCallView     = new QListWidget ( this              );
   m_pShowHistoCK  = new QCheckBox   ( this              );


   QStringList sortType;
   sortType << i18nc("Sort by name","Name") << i18nc("Sort by Organisation","Organisation") << i18nc("Sort by Recently used","Recently used") << i18nc("Sort by Group","Group") << i18nc("Sort by Department","Department");

   m_pSortByCBB->addItems(sortType);

   QWidget* mainWidget = new QWidget(this);
   setWidget(mainWidget);

   m_pContactView->headerItem()->setText             ( 0,i18n("Contacts") );
   m_pContactView->header()->setClickable            ( true               );
   m_pContactView->header()->setSortIndicatorShown   ( true               );
   m_pContactView->setAcceptDrops                    ( true               );
   m_pContactView->setDragEnabled                    ( true               );
   KeyPressEaterC *keyPressEater = new KeyPressEaterC( this               );
   m_pContactView->installEventFilter                ( keyPressEater      );

   m_pFilterLE->setPlaceholderText(i18n("Filter"));
   m_pFilterLE->setClearButtonShown(true);

   m_pShowHistoCK->setChecked(ConfigurationSkeleton::displayContactCallHistory());
   m_pShowHistoCK->setText(i18n("Display history"));

   setHistoryVisible(ConfigurationSkeleton::displayContactCallHistory());

   QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

   mainLayout->addWidget  ( m_pSortByCBB   );
   mainLayout->addWidget  ( m_pShowHistoCK );
   mainLayout->addWidget  ( m_pSplitter    );
   m_pSplitter->addWidget ( m_pContactView );
   m_pSplitter->addWidget ( m_pCallView    );
   mainLayout->addWidget  ( m_pFilterLE    );

   m_pSplitter->setChildrenCollapsible(true);
   m_pSplitter->setStretchFactor(0,7);

   QTimer* timer = new QTimer(this);

   m_pSortByCBB->setCurrentIndex(ConfigurationSkeleton::contactSortMode());

   connect(AkonadiBackend::getInstance() ,SIGNAL(collectionChanged()),                                  this, SLOT(reloadContact())                     );
   connect(m_pSortByCBB                  ,SIGNAL(currentIndexChanged(int)),                             this, SLOT(reloadContact())                     );
   connect(m_pContactView,                SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),this, SLOT(loadContactHistory(QTreeWidgetItem*)));
   connect(m_pFilterLE,                   SIGNAL(textChanged(QString)),                                 this, SLOT(filter(QString))                     );
   connect(m_pShowHistoCK,                SIGNAL(toggled(bool)),                                        this, SLOT(setHistoryVisible(bool))             );
   connect(timer                         ,SIGNAL(timeout()),                                            this, SLOT(reloadHistoryConst())                );
   connect(ConfigurationSkeleton::self() ,SIGNAL(configChanged()),                                      this, SLOT(reloadContact())                     );
   timer->start(1800*1000); //30 minutes
   setWindowTitle(i18n("Contact"));
} //ContactDock

///Destructor
ContactDock::~ContactDock()
{
   /*foreach (ContactItemWidget* w, m_Contacts) {
      delete w;
   }
   
   delete m_pFilterLE   ;
   delete m_pSplitter   ;
   delete m_pContactView;
   delete m_pCallView   ;
   delete m_pSortByCBB  ;
   delete m_pShowHistoCK;*/
}


/*****************************************************************************
 *                                                                           *
 *                                  Mutator                                  *
 *                                                                           *
 ****************************************************************************/

///Reload the contact
void ContactDock::reloadContact()
{
   ContactList list = AkonadiBackend::getInstance()->update();
   if (!list.size())
      return;
   m_pContactView->clear();
   m_Contacts.clear();

   QHash<Contact*, QDateTime> recentlyUsed;
   switch (CURRENT_SORTING_MODE) {
      case Recently_used:
         recentlyUsed = getContactListByTime();
         foreach (const QString& cat, m_slHistoryConst) {
            m_pContactView->addCategory(cat);
         }
         break;
   }

   foreach (Contact* cont, list) {
      if (cont->getPhoneNumbers().count() && usableNumberCount(cont)) {
         ContactItemWidget* aContact  = new ContactItemWidget(m_pContactView);
         QString category;
         switch (CURRENT_SORTING_MODE) {
            case Name:
               category = HelperFunctions::normStrippped(QString(cont->getFormattedName().trimmed()[0])).toUpper();
               break;
            case Organisation:
               category = (cont->getOrganization().isEmpty())?i18nc("Unknown category","Unknown"):cont->getOrganization();
               break;
            case Recently_used:
               if (recentlyUsed.find(cont) != recentlyUsed.end())
                  category = timeToHistoryCategory(recentlyUsed[cont].date());
               else
                  category = m_slHistoryConst[Never];
               break;
            case Group:
               category = i18n("TODO");
               break;
            case Department:
               category = (cont->getDepartment().isEmpty())?i18nc("Unknown category","Unknown"):cont->getDepartment();;
               break;
         }
         QNumericTreeWidgetItem_hist* item = m_pContactView->addItem<QNumericTreeWidgetItem_hist>(category);
         item->widget = aContact;
         aContact->setItem(item);
         aContact->setContact(cont);

         PhoneNumbers numbers =  aContact->getContact()->getPhoneNumbers();
         if (numbers.count() > 1) {
            foreach (Contact::PhoneNumber* number, numbers) {
               QNumericTreeWidgetItem_hist* item2 = new QNumericTreeWidgetItem_hist(item);
               item2->setFlags(item2->flags() | Qt::ItemIsDragEnabled);
               item2->number = number->getNumber();
               //Because of a Qt bug, we need to wrap the widget in an other widget, as drag and drop is broken for rich text (try dragging the bold part)
               QWidget* wrapper = new PhoneNumberItem(number->getNumber(),number->getType(),aContact->getContact()->getFormattedName(),this);
               m_pContactView->setItemWidget(item2,0,wrapper);
            }
         }
         else if (numbers.count() == 1) {
            item->number = numbers[0]->getNumber();
         }

         m_pContactView->setItemWidget(item,0,aContact);
         m_Contacts << aContact;
      }
   }
   switch (CURRENT_SORTING_MODE) {
      case Recently_used:
         break;
      default:
         m_pContactView->sortItems(0,Qt::AscendingOrder);
   }

   ConfigurationSkeleton::setContactSortMode(m_pSortByCBB->currentIndex());

   if (ConfigurationSkeleton::alwaysShowPhoneNumber()) {
      m_pContactView->expandAll();
   }
} //reloadContact

///Query the call history for all items related to this contact
void ContactDock::loadContactHistory(QTreeWidgetItem* item)
{
   if (m_pShowHistoCK->isChecked()) {
      m_pCallView->clear();
      if (dynamic_cast<QNumericTreeWidgetItem_hist*>(item) != nullptr) {
         QNumericTreeWidgetItem_hist* realItem = dynamic_cast<QNumericTreeWidgetItem_hist*>(item);
         foreach (Call* call, HistoryModel::getHistory()) {
            if (realItem->widget != 0) {
               foreach (Contact::PhoneNumber* number, realItem->widget->getContact()->getPhoneNumbers()) {
                  if (number->getNumber() == call->getPeerPhoneNumber()) {
                     m_pCallView->addItem(QDateTime::fromTime_t(call->getStartTimeStamp().toUInt()).toString());
                  }
               }
            }
         }
      }
   }
} //loadContactHistory

///Filter contact
void ContactDock::filter(const QString& text)
{
   foreach(ContactItemWidget* item, m_Contacts) {
      bool foundNumber = false;
      foreach (Contact::PhoneNumber* number, item->getContact()->getPhoneNumbers()) {
         foundNumber |= number->getNumber().toLower().indexOf(text.toLower()) != -1;
      }
      bool visible = (HelperFunctions::normStrippped(item->getContact()->getFormattedName  ()).indexOf(HelperFunctions::normStrippped(text)) != -1)
                  || (HelperFunctions::normStrippped(item->getContact()->getOrganization   ()).indexOf(HelperFunctions::normStrippped(text)) != -1)
                  || (HelperFunctions::normStrippped(item->getContact()->getPreferredEmail ()).indexOf(HelperFunctions::normStrippped(text)) != -1)
                  || (HelperFunctions::normStrippped(item->getContact()->getDepartment     ()).indexOf(HelperFunctions::normStrippped(text)) != -1)
                  || foundNumber;
      item->getItem()->setHidden(!visible);

   }

   for (int i=0;i< m_pContactView->topLevelItemCount();i++) {
      bool visible = false;
      QTreeWidgetItem* item = m_pContactView->topLevelItem(i);
      int count  = item->childCount();
      for (int j=0;j<count;j++) visible |= !item->child(j)->isHidden();
      m_pContactView->topLevelItem(i)->setHidden(!visible);
   }
} //filter

void ContactDock::reloadHistoryConst()
{
   switch (CURRENT_SORTING_MODE) {
      case Recently_used:
         reloadContact();
         break;
   }
}

/*****************************************************************************
 *                                                                           *
 *                                Drag and Drop                              *
 *                                                                           *
 ****************************************************************************/

///Serialize information to be used for drag and drop
QMimeData* ContactTree::mimeData( const QList<QTreeWidgetItem *> items) const
{
   kDebug() << "An history call is being dragged";
   if (items.size() < 1) {
      return nullptr;
   }

   QMimeData *mimeData = new QMimeData();

   //Contact
   if (dynamic_cast<QNumericTreeWidgetItem_hist*>(items[0])) {
      QNumericTreeWidgetItem_hist* item = dynamic_cast<QNumericTreeWidgetItem_hist*>(items[0]);
      if (item->widget != 0) {
         mimeData->setData(MIME_CONTACT, item->widget->getContact()->getUid().toUtf8());
      }
      else if (!item->number.isEmpty()) {
         mimeData->setData(MIME_PHONENUMBER, item->number.toUtf8());
      }
   }
   else {
      kDebug() << "the item is not a call";
   }
   return mimeData;
} //mimeData

///Handle data being dropped on the widget
bool ContactTree::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
   Q_UNUSED(index )
   Q_UNUSED(action)
   Q_UNUSED(parent)

   QByteArray encodedData = data->data(MIME_CALLID);

   kDebug() << "In history import"<< QString(encodedData);

   return false;
}


/*****************************************************************************
 *                                                                           *
 *                                  Setters                                  *
 *                                                                           *
 ****************************************************************************/

///Show or hide the history list
void ContactDock::setHistoryVisible(bool visible)
{
   kDebug() << "Toggling history visibility";
   m_pCallView->setVisible(visible);
   ConfigurationSkeleton::setDisplayContactCallHistory(visible);
}


/*****************************************************************************
 *                                                                           *
 *                             Keyboard handling                             *
 *                                                                           *
 ****************************************************************************/

///Handle keypresses ont the dock
void ContactDock::keyPressEvent(QKeyEvent* event) {
   int key = event->key();
   if(key == Qt::Key_Escape)
      m_pFilterLE->setText(QString());
   else if(key == Qt::Key_Return || key == Qt::Key_Enter) {
      if (m_pContactView->selectedItems().size() && m_pContactView->selectedItems()[0] && m_pContactView->itemWidget(m_pContactView->selectedItems()[0],0)) {
         QNumericTreeWidgetItem_hist* item = dynamic_cast<QNumericTreeWidgetItem_hist*>(m_pContactView->selectedItems()[0]);
         if (item) {
            Call* call = nullptr;
            SFLPhone::app()->view()->selectCallPhoneNumber(&call,item->widget->getContact());
         }
      }
   }
   else if((key == Qt::Key_Backspace) && (m_pFilterLE->text().size()))
      m_pFilterLE->setText(m_pFilterLE->text().left( m_pFilterLE->text().size()-1 ));
   else if (!event->text().isEmpty() && !(key == Qt::Key_Backspace))
      m_pFilterLE->setText(m_pFilterLE->text()+event->text());
} //keyPressEvent

/*****************************************************************************
 *                                                                           *
 *                                  Helpers                                  *
 *                                                                           *
 ****************************************************************************/
