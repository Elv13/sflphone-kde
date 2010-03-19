/***************************************************************************
 *   Copyright (C) 2009 by Savoir-Faire Linux                              *
 *   Author : Jérémy Quentin                                               *
 *   jeremy.quentin@savoirfairelinux.com                                   *
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
 ***************************************************************************/
#ifndef DLGACCOUNTS_H
#define DLGACCOUNTS_H

#include <QWidget>
#include <kconfigdialog.h>

#include "ui_dlgaccountsbase.h"
#include "Account.h"
#include "AccountList.h"

/**
	@author Jérémy Quentin <jeremy.quentin@gmail.com>
	
	\note see ticket #1309 for advices about how to improve this class.
*/
class DlgAccounts : public QWidget, public Ui_DlgAccountsBase
{
Q_OBJECT
public:
	DlgAccounts(KConfigDialog *parent = 0);

	void saveAccount(QListWidgetItem * item);
	
	/**
	 *   Fills the settings form in the right side with the
	 *   settings of @p item.
	 *
	 *   \note When the user creates a new account, its accountDetails
	 *   map is empty, so the form is filled with blank strings,
	 *   zeros... And when the user clicks \e Apply , these settings are
	 *   saved just after the account is created. So be careful the form
	 *   is filled with the right default settings if blank (as 600 for
	 *   registration expire).
	 *
	 * @param item the item with which to fill the settings form
	 */
	void loadAccount(QListWidgetItem * item);
	
private:
	AccountList * accountList;
	bool accountListHasChanged;

public slots:
	void saveAccountList();
	void loadAccountList();
	
	bool hasChanged();
	void updateSettings();
	void updateWidgets();
	
private slots:
	void changedAccountList();
	void connectAccountsChangedSignal();
	void disconnectAccountsChangedSignal();
	void on_button_accountUp_clicked();
	void on_button_accountDown_clicked();
	void on_button_accountAdd_clicked();
	void on_button_accountRemove_clicked();
	void on_edit1_alias_textChanged(const QString & text);
	void on_listWidget_accountList_currentItemChanged ( QListWidgetItem * current, QListWidgetItem * previous );
	//void on_toolButton_accountsApply_clicked(); //Disabled for future removal
	void updateAccountStates();
	void addAccountToAccountList(Account * account);
	void updateAccountListCommands();
	void updateStatusLabel(QListWidgetItem * item);
	void updateStatusLabel(Account * account);
	
	
signals:
	void updateButtons();

};

#endif
