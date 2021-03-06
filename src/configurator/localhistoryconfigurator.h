/***************************************************************************
 *   Copyright (C) 2015 by Savoir-Faire Linux                              *
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
#ifndef LOCALHISTORYCONFIGURATOR_H
#define LOCALHISTORYCONFIGURATOR_H

#include <collectionconfigurationinterface.h>

class DlgGeneral;

class LocalHistoryConfigurator : public CollectionConfigurationInterface
{
   Q_OBJECT
public:
   LocalHistoryConfigurator(QObject* parent = nullptr);

   //Getter
   virtual QByteArray id  () const override;
   virtual QString    name() const override;
   virtual QVariant   icon() const override;

   //Mutator

   /**
    * This function will be called when a collection request to be configured
    * 
    * @param col The collection to be edited. It can casted
    * @param parent can be used for layout information.
    */
   virtual void loadCollection(CollectionInterface* col, QObject* parent = nullptr) override;

private:
   DlgGeneral* m_pDialog;

};

#endif
