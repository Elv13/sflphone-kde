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
#include "dlgaudio.h"

//Qt
#include <QtGui/QHeaderView>

//KDE
#include <KStandardDirs>
#include <KLineEdit>

//SFLPhone
#include "lib/configurationmanager_interface_singleton.h"
#include "klib/configurationskeleton.h"
#include "conf/configurationdialog.h"
#include "lib/sflphone_const.h"

///Constructor
DlgAudio::DlgAudio(KConfigDialog *parent)
 : QWidget(parent)
{
   setupUi(this);

   ConfigurationManagerInterface& configurationManager = ConfigurationManagerInterfaceSingleton::getInstance();

   KUrlRequester_destinationFolder->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
   KUrlRequester_destinationFolder->setUrl(KUrl(configurationManager.getRecordPath()));
   KUrlRequester_destinationFolder->lineEdit()->setObjectName("kcfg_destinationFolder");
   KUrlRequester_destinationFolder->lineEdit()->setReadOnly(true);

   connect( box_alsaPlugin, SIGNAL(activated(int)),  parent, SLOT(updateButtons()));
   connect( this,           SIGNAL(updateButtons()), parent, SLOT(updateButtons()));
}

///Destructor
DlgAudio::~DlgAudio()
{
}

///Update the widgets
void DlgAudio::updateWidgets()
{
   loadAlsaSettings();
}

///Save the settings
void DlgAudio::updateSettings()
{
   //alsaPlugin
   ConfigurationSkeleton * skeleton = ConfigurationSkeleton::self();
   skeleton->setAlsaPlugin(box_alsaPlugin->currentText());
   
   ConfigurationManagerInterface& configurationManager = ConfigurationManagerInterfaceSingleton::getInstance();
   configurationManager.setRecordPath(KUrlRequester_destinationFolder->lineEdit()->text());
}

///Have this dialog changed
bool DlgAudio::hasChanged()
{
   ConfigurationSkeleton * skeleton = ConfigurationSkeleton::self();
   bool alsaPluginHasChanged = skeleton->interface() == ConfigurationSkeleton::EnumInterface::ALSA && skeleton->alsaPlugin() != box_alsaPlugin->currentText();
   return alsaPluginHasChanged ;
}

///Load alsa settings
void DlgAudio::loadAlsaSettings()
{
   ConfigurationManagerInterface& configurationManager = ConfigurationManagerInterfaceSingleton::getInstance();
   if(QString(configurationManager.getAudioManager()) == "alsa") {
      ConfigurationSkeleton* skeleton = ConfigurationSkeleton::self();

      int index = box_alsaPlugin->findText(skeleton->alsaPlugin());
      if(index < 0) index = 0;
      QStringList pluginList       = configurationManager.getAudioPluginList       ();
      box_alsaPlugin->clear                 (                              );
      box_alsaPlugin->addItems              ( pluginList                   );
      box_alsaPlugin->setCurrentIndex       ( index                        );

      QStringList inputDeviceList  = configurationManager.getAudioInputDeviceList  ();
      kcfg_alsaInputDevice->clear           (                              );
      kcfg_alsaInputDevice->addItems        ( inputDeviceList              );
      kcfg_alsaInputDevice->setCurrentIndex ( skeleton->alsaInputDevice()  );

      QStringList outputDeviceList = configurationManager.getAudioOutputDeviceList ();
      kcfg_alsaOutputDevice->clear          (                              );
      kcfg_alsaOutputDevice->addItems       ( outputDeviceList             );
      kcfg_alsaOutputDevice->setCurrentIndex( skeleton->alsaOutputDevice() );

      groupBox_alsa->setEnabled(true);
   }
   else {
      groupBox_alsa->setEnabled(false);
   }
}