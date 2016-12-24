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

import xbmc
import xbmcaddon
import xbmcgui
import string

addon         = xbmcaddon.Addon()
resourcespath = xbmc.translatePath(addon.getAddonInfo('path')) + '/resources'
gChannelAlias = {}

sys.path.insert(0, resourcespath + '/lib')
import graphlcd


# http://kodi.wiki/view/Window_IDs
class WINDOW_IDS:
  WINDOW_HOME                  = 10000
  WINDOW_VIDEO_NAV             = 10025
  WINDOW_MUSIC_NAV             = 10502
  WINDOW_FULLSCREEN_VIDEO      = 12005
  WINDOW_VISUALISATION         = 12006
  WINDOW_TV_CHANNELS_OLD       = 10615 # Jarvis
  WINDOW_TV_RECORDINGS_OLD     = 10616
  WINDOW_RADIO_CHANNELS_OLD    = 10620
  WINDOW_TV_CHANNELS           = 10700
  WINDOW_TV_RECORDINGS         = 10701
  WINDOW_RADIO_CHANNELS        = 10705

# Returns "cleaned up" volume value as plain integer from 0 (min) to 60 (max)
def GetPlayerVolume():
  return 60 + int(float(string.replace(string.replace(xbmc.getInfoLabel('Player.Volume'), ',', '.'), ' dB', '')))

# Returns "cleaned up" time values (integer only, in seconds)
def GetTime(aVariable):
  timestr = xbmc.getInfoLabel(aVariable + '(hh:mm:ss)')
  parts = timestr.split(':')
  if len(parts) != 3:
    return ""
  return int(parts[0]) * 60 * 60 + int(parts[1]) * 60 + int(parts[2])


# Reads "channels.alias" file. Fills global "gChannelAlias" variable
def ReadChannelsAlias():
  fh = open(resourcespath + '/channels.alias', 'r')
  for line in fh:
    parts = line.strip().split(':')
    if len(parts) == 2:
      gChannelAlias[parts[0]] = parts[1]


# Returns the name of the screen which should appear on the LCD in Kodi's
# current state
def GetCurrentScreenName():
  windowid = xbmcgui.getCurrentWindowId()
  sys.stderr.write('Window ID: ' + str(windowid) + "\n")
  sys.stderr.write('NumItems: ' + xbmc.getInfoLabel('Container.NumItems') + "\n")
  if windowid == WINDOW_IDS.WINDOW_HOME:
    return 'navigation'
  if windowid == WINDOW_IDS.WINDOW_MUSIC_NAV or \
     windowid == WINDOW_IDS.WINDOW_VIDEO_NAV or \
     windowid == WINDOW_IDS.WINDOW_TV_CHANNELS_OLD or \
     windowid == WINDOW_IDS.WINDOW_TV_RECORDINGS_OLD or \
     windowid == WINDOW_IDS.WINDOW_RADIO_CHANNELS_OLD or \
     windowid == WINDOW_IDS.WINDOW_TV_CHANNELS or \
     windowid == WINDOW_IDS.WINDOW_TV_RECORDINGS or \
     windowid == WINDOW_IDS.WINDOW_RADIO_CHANNELS:
    return 'menu'
  if windowid == WINDOW_IDS.WINDOW_VISUALISATION or \
     windowid == WINDOW_IDS.WINDOW_FULLSCREEN_VIDEO:
    if xbmc.getCondVisibility('Pvr.IsPlayingTv') or \
       xbmc.getCondVisibility('Pvr.IsPlayingRadio'):
      return 'tvshow'
    else:
      return 'replay'
  else:
    return 'general'

# Returns name of the screen which is to be drawn above the current main screen
# Currently only for the "volume" overlay
def GetCurrentOverlayName():
  volume = GetPlayerVolume()
  if not hasattr(GetCurrentOverlayName, 'lastvolume'):
    GetCurrentOverlayName.lastvolume = volume

  if GetCurrentOverlayName.lastvolume != volume or \
     xbmc.getCondVisibility('Player.Muted'):
    GetCurrentOverlayName.lastvolume = volume
    return 'volume'
  else:
    return ''


# The C(++) part calls into this callback function to get values for token names
def GetTokenValue(aVariableName, aAttrib, aIndex, aMaxItems):
  # Kodi "Builtins" (Prefixed with "Info." or "Bool.")
  if aVariableName.startswith('Info.'):
    if aAttrib != '':
      return xbmc.getInfoLabel(aVariableName[5:] + '(' + aAttrib + ')')
    else:
      return xbmc.getInfoLabel(aVariableName[5:])
  elif aVariableName.startswith('Bool.'):
    return xbmc.getCondVisibility(aVariableName[5:])

  # Volume level variables
  elif aVariableName == 'VolumeCurrent':
    return GetPlayerVolume()
  elif aVariableName == 'VolumeTotal':
    return 60

  # Playback times
  elif aVariableName == 'PlayerDuration':
    return GetTime('Player.Duration')
  elif aVariableName == 'PlayerTime':
    return GetTime('Player.Time')

  # Scroll settings
  elif aVariableName == 'ScrollMode' or \
       aVariableName == 'ScrollSpeed' or \
       aVariableName == 'ScrollTime':
    return addon.getSetting(aVariableName.lower())

  # Channel alias
  elif aVariableName == 'ChannelAlias':
    channelname = xbmc.getInfoLabel('VideoPlayer.ChannelName')
    if channelname in gChannelAlias:
      return gChannelAlias[channelname]
    else:
      return 0

  # Menu variables
  elif aVariableName == 'MenuItem' or \
       aVariableName == 'IsMenuCurrent':
    infolabelcuritem = xbmc.getInfoLabel('Container.CurrentItem')
    infolabelnumitems = xbmc.getInfoLabel('Container().NumItems')
    # Empty values happen if the menu is closing or opening.
    if infolabelcuritem == '' or infolabelnumitems == '':
      return ''
    osdCurrentItemIndex = int(infolabelcuritem) - 1
    osdItemsSize = int(infolabelnumitems)

    # "Folder lists" have a leading ".." item.
    if xbmc.getCondVisibility('Container.HasParent'):
      osdCurrentItemIndex += 1
      osdItemsSize += 1

    if osdItemsSize == 0:
      return ''

    maxItems = aMaxItems
    if maxItems > osdItemsSize:
      maxItems = osdItemsSize

    currentIndex = maxItems / 2
    if (osdCurrentItemIndex < currentIndex):
      currentIndex = osdCurrentItemIndex

    topIndex = osdCurrentItemIndex - currentIndex
    if (topIndex + maxItems) > osdItemsSize:
      currentIndex += (topIndex + maxItems) - osdItemsSize
      topIndex = osdCurrentItemIndex - currentIndex

    if aVariableName == 'MenuItem':
      if aIndex < maxItems:
        return xbmc.getInfoLabel('Container().ListItemAbsolute(' + str(topIndex + aIndex) + ').Label')

    if aVariableName == 'IsMenuCurrent':
      if aIndex < maxItems and aIndex == currentIndex:
        return 1

    return ''

  else:
    sys.stderr.write('Graphlcd: Invalid variable name requested: ' + aVariableName + "\n")
    return ''

if __name__ == '__main__':
  graphlcd.SetResourcePath(resourcespath)

  ReadChannelsAlias()

  loaded_driver = 0
  loaded_skin = 0
  config_loaded = 0
  wait_time = 0.1
  monitor = xbmc.Monitor()
  while not monitor.abortRequested():
    if monitor.waitForAbort(wait_time):
      break
    wait_time = 0.5

    # Try to load config if not already done
    if not config_loaded:
      sys.stderr.write("Loading config\n");
      config_loaded = graphlcd.ConfigLoad()
      if not config_loaded:
        sys.stderr.write("Graphlcd: Can't read graphlcd.conf\n")
        wait_time = 1
        continue

    # Load driver
    driver_setting = addon.getSetting('driver')
    if loaded_driver != driver_setting:
      sys.stderr.write("Loading driver\n");
      if graphlcd.CreateDriver(driver_setting):
        loaded_driver = driver_setting
        loaded_skin = 0 # Reload skin if driver has changed!
      else:
        sys.stderr.write('Graphlcd: Failed to create driver ' + driver_setting + "\n")
        wait_time = 1
        continue

    # Load skin
    skin_setting = addon.getSetting('skin')
    if loaded_skin != skin_setting:
      sys.stderr.write("Loading skin\n");
      if graphlcd.ParseSkin(skin_setting):
        loaded_skin = skin_setting
      else:
        sys.stderr.write('Graphlcd: Failed to load skin ' + skin_setting + "\n")
        continue

    # Render display
    if not graphlcd.Render(GetCurrentScreenName(), GetCurrentOverlayName(), GetTokenValue):
      sys.stderr.write("Graphlcd: Failed to render display\n")


  # Before stopping Addon, send the "shutdown screen" to the LCD.
  # Only do this if display and skin are loaded.
  # Use a lambda callback to prevent calling back into Kodi while shutting down
  if loaded_skin:
    graphlcd.Render('shutdown', '', lambda x1, x2, x3, x4: 0)

  # Finally shutdown graphlcd
  graphlcd.Shutdown()
