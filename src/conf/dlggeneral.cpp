/****************************************************************************
 *   Copyright (C) 2009 by Savoir-Faire Linux                               *
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
#include "dlggeneral.h"

//Qt
#include <QtGui/QToolButton>
#include <QtGui/QAction>

//SFLPhone
#include "klib/configurationskeleton.h"
#include "conf/configurationdialog.h"
#include "lib/configurationmanager_interface_singleton.h"

///Constructor
DlgGeneral::DlgGeneral(KConfigDialog *parent)
 : QWidget(parent),m_HasChanged(false)
{
   setupUi(this);
   connect(toolButton_historyClear, SIGNAL(clicked()), this, SIGNAL(clearCallHistoryAsked()));
   toolButton_historyClear->setIcon(KIcon("edit-clear-history"));

   kcfg_historyMax->setValue(ConfigurationSkeleton::historyMax());
   connect(this             , SIGNAL(updateButtons())                , parent, SLOT(updateButtons()));
}

///Destructor
DlgGeneral::~DlgGeneral()
{
}

///Have this dialog changed
bool DlgGeneral::hasChanged()
{
   return m_HasChanged;
}

///Tag this dialog as changed
void DlgGeneral::changed()
{
   m_HasChanged = true;
   emit updateButtons();
}

///Update all widgets
void DlgGeneral::updateWidgets()
{
}

///Save current settings
void DlgGeneral::updateSettings()
{
//    QMutableMapIterator<QString, QString> iter(m_lCallDetails);
//    while (iter.hasNext()) {
//       iter.next();
//       ConfigurationSkeleton::self()->findItem(iter.value())->setProperty(m_lItemList[iter.value()]->checkState() == Qt::Checked);
//    }
   ConfigurationSkeleton::setHistoryMax(kcfg_historyMax->value());

   m_HasChanged = false;
}
