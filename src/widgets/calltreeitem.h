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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/
#ifndef CALLTREE_ITEM_H
#define CALLTREE_ITEM_H

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtGui/QWidget>

//Qt
class QLabel;
class QPushButton;
class QMimeData;
class QTimer;
class QPixmap;

//KDE
class KIcon;

//SFLPhone
class Call;
class TranslucentButtons;
class Contact;

///CallTreeItem: Widget for the central call treeview
class CallTreeItem : public QWidget
{
   Q_OBJECT
 public:
    //Constructor
    CallTreeItem(QWidget* parent =0);
    ~CallTreeItem();

    //Getters
    Call*         call()      const;
    virtual QSize sizeHint () const;

    //Setters
    void setCall(Call *call);

    //Const
    static const char* callStateIcons[12];

 private:
    //Attributes
    Call*    m_pItemCall        ;
    bool     m_Init             ;
    bool     m_isHover          ;
    QLabel*  m_pIconL           ;
    QLabel*  m_pPeerL           ;
    QLabel*  m_pCallNumberL     ;
    QLabel*  m_pTransferPrefixL ;
    QLabel*  m_pTransferNumberL ;
    QLabel*  m_pCodecL          ;
    QLabel*  m_pSecureL         ;
    QLabel*  m_pHistoryPeerL    ;
    QLabel*  m_pElapsedL        ;
    QLabel*  m_pDepartment      ;
    QLabel*  m_pOrganisation    ;
    QLabel*  m_pEmail           ;
    QTimer*  m_pTimer           ;
    uint     m_Height           ;

    Contact* m_pContact;
    
    TranslucentButtons* m_pBtnConf ;
    TranslucentButtons* m_pBtnTrans;

    //Helper
    QPixmap& applyMask(QPixmap& pxm);

  protected:
    //Reimplementation
    virtual void dragEnterEvent        ( QDragEnterEvent *e );
    virtual void dragMoveEvent         ( QDragMoveEvent  *e );
    virtual void dragLeaveEvent        ( QDragLeaveEvent *e );
    virtual void resizeEvent           ( QResizeEvent    *e );
    virtual void dropEvent             ( QDropEvent      *e );
    virtual void mouseDoubleClickEvent ( QMouseEvent     *e );

private slots:
   void transferEvent    (QMimeData* data);
   void conversationEvent(QMimeData* data);
   void hide             (               );
   void incrementTimer   (               );
   void copy             (               );

public slots:
   void updated();

signals:
   ///Emitted when a call is over
   void over(Call*);
   ///Emitted when the item change
   void changed();
   ///Emitted when it is required to expand the childrens
   void showChilds(CallTreeItem*);
   ///Emitted when a call is dropped on the "transfer" button
   void askTransfer(Call*);
   ///Emitted when something is dropped on the button
   void transferDropEvent(Call*,QMimeData*);
   ///Emitted when something is dropped on the "conference" button
   void conversationDropEvent(Call*,QMimeData*);
 };

#endif // CALLTREE_ITEM_H