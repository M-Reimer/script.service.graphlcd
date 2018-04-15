#  Graphlcd Addon for Kodi
#  Copyright (C) 2018 Manuel Reimer <manuel.reimer@gmx.de>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

import xbmc

_gCurrentItemIndex = 0;
_gItemsSize = 0;

def Update():
  global _gCurrentItemIndex
  global _gItemsSize
  infolabelcuritem = xbmc.getInfoLabel('Container().CurrentItem')
  infolabelnumitems = xbmc.getInfoLabel('Container().NumItems')

  # Empty values happen if the menu is closing or opening.
  if infolabelcuritem == '' or infolabelnumitems == '':
    _gCurrentItemIndex = 0
    _gItemsSize = 0
    return

  _gCurrentItemIndex = int(infolabelcuritem) - 1
  _gItemsSize = int(infolabelnumitems)

  # "Folder lists" have a leading ".." item.
  if xbmc.getCondVisibility('Container.HasParent'):
    _gCurrentItemIndex += 1
    _gItemsSize += 1

def GetCurrentItemIndex():
  return _gCurrentItemIndex

def GetItemsSize():
  return _gItemsSize
