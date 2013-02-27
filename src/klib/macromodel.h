/****************************************************************************
 *   Copyright (C) 2013 by Savoir-Faire Linux                               *
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

#ifndef MACRO_MODEL_H
#define MACRO_MODEL_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QHash>
#include "../lib/typedefs.h"

//KDE
class KAction;

//SFLPhone

class Macro;

///Interface to interpret DTMFs instead of using the daemon directly
class LIB_EXPORT MacroListener {
public:
   virtual void addDTMF(const QString& sequence) = 0;
   virtual ~MacroListener() {}
};

///AkonadiBackend: Implement a backend for Akonadi
class LIB_EXPORT MacroModel : public QAbstractItemModel {
   Q_OBJECT
   friend class Macro;

private:
   
   enum IndexType {
      CategoryIndex = 1,
      MacroIndex = 2
   };
   
   struct IndexPointer {
      IndexPointer(IndexType _type, void* _data) : type(_type),data(_data) {}
      IndexType type;
      void* data;
   };

   struct MacroCategory {
      MacroCategory():m_pPointer(nullptr){}
      QString m_Name;
      QList<Macro*> m_lContent;
      IndexPointer* m_pPointer;
   };
public:
   static MacroModel* getInstance();
   static void addListener(MacroListener* interface);

   enum MacroFields {
      Name        = 100,
      Category    = 101,
      Delay       = 102,
      Description = 103,
      Sequence    = 104
   };

   void initMacros();

   //Getters
   Macro* getCurrentMacro();

   //Mutator
   MacroModel::MacroCategory* createCategory(const QString& name);

   //Model implementation
   virtual bool          setData     ( const QModelIndex& index, const QVariant &value, int role   );
   virtual QVariant      data        ( const QModelIndex& index, int role = Qt::DisplayRole        ) const;
   virtual int           rowCount    ( const QModelIndex& parent = QModelIndex()                   ) const;
   virtual Qt::ItemFlags flags       ( const QModelIndex& index                                    ) const;
   virtual int           columnCount ( const QModelIndex& parent = QModelIndex()                   ) const;
   virtual QModelIndex   parent      ( const QModelIndex& index                                    ) const;
   virtual QModelIndex   index       ( int row, int column, const QModelIndex& parent=QModelIndex()) const;
   virtual QVariant      headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

private:
   //Singleton constructor
   explicit MacroModel(QObject* parent = nullptr);

   //Attributes
   static MacroModel*         m_pInstance  ;

private:
   void updateTreeModel(Macro* newMacro);
   QHash<QString,Macro*> m_hMacros;
   QList<MacroCategory*> m_lCategories;
   QList<MacroListener*> m_lListeners;
   Macro*                m_pCurrentMacro;
   Macro*                m_pCurrentMacroMemento;

public Q_SLOTS:
   Macro* newMacro(const QString& id = "");
   bool removeMacro(QModelIndex idx);
   void setCurrent(QModelIndex current,QModelIndex previous);
   void save();

private Q_SLOTS:
   void changed(Macro* macro);

Q_SIGNALS:
   void addAction(KAction*);
   void selectMacro(Macro* macro);
};

class LIB_EXPORT Macro : public QObject {
   Q_OBJECT
   friend class MacroModel; //Use factory method
public:
   Macro(const Macro* macro);
   MacroModel::MacroCategory* m_pCat;
   //Getters
   QString  name();
   QString  description();
   QString  sequence();
   QString  escaped();
   QString  id();
   int      delay();
   QString  category();
   KAction* action();

   QModelIndex index();

   //Setters
   void setName(QString value);
   void setDescription(QString value);
   void setSequence(QString value);
   void setEscaped(QString value);
   void setId(QString value);
   void setDelay(int value);
   void setCategory(QString value);
   
private:
   explicit Macro(QObject* parent = nullptr);
   int         m_Position;
   QString     m_Name;
   QString     m_Description;
   QString     m_Sequence;
   QString     m_Escaped;
   QString     m_Id;
   int         m_Delay;
   QString     m_Category;
   KAction*    m_Action;
   MacroModel* m_pModel;
   MacroModel::IndexPointer* m_pPointer;
public Q_SLOTS:
   void execute();
private Q_SLOTS:
   void nextStep();

Q_SIGNALS:
   void changed(Macro*);
};

#endif