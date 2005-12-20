/**
 *  Copyright (C) 2004-2005 Savoir-Faire Linux inc.
 *  Author: Jean-Philippe Barrette-LaPierre
 *             <jean-philippe.barrette-lapierre@savoirfairelinux.com>
 *                                                                              
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *                                                                              
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                              
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef SFLPHONEGUI_OBJECTPOOL_H
#define SFLPHONEGUI_OBJECTPOOL_H

#include <list>
#include <qstring.h>
#include <QMutex>
#include <QWaitCondition>

template< typename T >
class ObjectPool
{
 public:
  /**
   * This function will push a line in the pool.
   */
  void push(const T &line);

  /**
   * This function will wait for an available line.
   */
  bool pop(T &value, unsigned long time = ULONG_MAX);

 private:
  std::list< T > mPool;

  QMutex mMutex;
  QWaitCondition mDataAvailable;
};

#include "ObjectPool.inl"

#endif

