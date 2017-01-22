#  Graphlcd Addon for Kodi
#  Copyright (C) 2016 Manuel Reimer <manuel.reimer@gmx.de>
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

_gChannelAlias = {}

def Load(aPath):
  fh = open(aPath)
  for line in fh:
    parts = line.strip().split(':')
    if len(parts) == 2:
      _gChannelAlias[parts[0]] = parts[1]

def GetChannelAlias(aChannelName):
  if aChannelName in _gChannelAlias:
    return _gChannelAlias[aChannelName]
  else:
    return 0
