/***************************************************************************
 *   Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).     *
 *   All rights reserved.                                                  *
 *   Contact: Nokia Corporation (qt-info@nokia.com)                        *
 *   Author : Mathieu Leduc-Hamel mathieu.leduc-hamel@savoirfairelinux.com *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 **************************************************************************/

/**
 * http://doc.trolltech.com/4.5/itemviews-editabletreemodel.html
 */

#ifndef HISTORYTREE_ITEM_H
#define HISTORYTREE_ITEM_H

#include <QtGui/QWidget>
#include <QtCore/QList>
#include <Phonon/MediaObject>

//SFLPhone
class Call;
class Contact;

//Qt
class QTreeWidgetItem;
class QMenu;
class QLabel;
class QToolButton;
class QGridLayout;

namespace Phonon{
   class SeekSlider;
   class MediaObject;
   class AudioOutput;
   class MediaSource;
}

//KDE
class KAction;

///@class HistoryTreeItem Items for the history dock
class HistoryTreeItem : public QWidget
{
   Q_OBJECT
 public:
    //Constructor
    HistoryTreeItem(QWidget* parent =0, QString phone = "");
    ~HistoryTreeItem();

    //Getters
    Call*            call           () const;
    uint             getTimeStamp   ();
    uint             getLength    ();
    QString          getName        ();
    QString          getPhoneNumber ();
    QTreeWidgetItem* getItem        ();
    uint             getDurWidth    ();

    //Setters
    void setCall     ( Call*            call  );
    void setItem     ( QTreeWidgetItem* item  );
    void setDurWidth ( uint             width );

    //Const
    static const char * callStateIcons[12];

 private:
    //Attributes
    Call*        m_pItemCall      ;

    QLabel*      m_pIconL         ;
    QLabel*      m_pPeerNameL     ;
    QLabel*      m_pCallNumberL   ;
    QLabel*      m_pTimeL         ;
    QLabel*      m_pLengthL     ;

    KAction*     m_pCallAgain     ;
    KAction*     m_pAddContact    ;
    KAction*     m_pAddToContact  ;
    KAction*     m_pCopy          ;
    KAction*     m_pEmail         ;
    KAction*     m_pBookmark      ;
    QMenu*       m_pMenu          ;

    QToolButton* m_pPlay          ;
    QToolButton* m_pRemove        ;

    uint         m_TimeStamp      ;
    uint         m_Length       ;
    QString      m_Name           ;
    QString      m_PhoneNumber    ;
    QGridLayout* m_pMainLayout    ;
    Contact*     m_pContact;

    QTreeWidgetItem* m_pItem;

    Phonon::MediaObject*       m_pMediaObject             ;
    Phonon::MediaObject*       m_pMetaInformationResolver ;
    Phonon::AudioOutput*       m_pAudioOutput             ;
    QList<Phonon::MediaSource> m_lSources                 ;

    //Recorded call player
    Phonon::SeekSlider* m_pAudioSlider ;
    QLabel*             m_pTimeLeftL   ;
    QLabel*             m_pTimePlayedL ;
    QToolButton*        m_pPause       ;
    QToolButton*        m_pStop        ;
    QToolButton*        m_pNote        ;
    QWidget*            m_pPlayer      ;

protected:
   virtual void resizeEvent(QResizeEvent* event);

public slots:
   void updated();
   bool getContactInfo(QString phone);

private slots:
   void showContext(const QPoint& pos);
   void sendEmail();
   void callAgain();
   void copy();
   void addContact();
   void addToContact();
   void bookmark();
   void removeRecording();
   void showRecordPlayer();
   void stopPlayer();
   void playPausePlayer();
   void editNote();
   void metaStateChanged(Phonon::State newState, Phonon::State oldState);
   void tick(qint64 time);
   void stateChanged(Phonon::State newState, Phonon::State /* oldState */);

signals:
   void over(Call*);
};

#endif // CALLTREE_ITEM_H
