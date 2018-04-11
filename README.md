Graphlcd Addon for Kodi
=======================

This Kodi Addon allows to drive a graphical LCD with Kodi, allowing to navigate the menu and basic lists and to view information about the current playback.

This Addon requires graphlcd-base to be installed and it is recommended to also install serdisplib, which is an optional dependency but brings some more LCD drivers. Both dependencies can be found via the following links:

* https://projects.vdr-developer.org/git/graphlcd-base.git (use the latest GIT version! The last release will not work properly with this Kodi Addon!)
* http://serdisplib.sourceforge.net/

A list of supported displays can be found on the project homepages. As currently there is no USB LCD available on the market, you may also be interested in my [USBserLCD](https://github.com/M-Reimer/usbserlcd) project, which provides Arduino firmware to drive a T6963C based LCD module via USB.

![Kodi Graphlcd output](https://raw.githubusercontent.com/M-Reimer/script.service.graphlcd/master/icon.png "Kodi Graphlcd output")

Setup
-----

Installation for only one user (currently logged in user):

    cd ~/.kodi/addons
    git clone https://github.com/M-Reimer/script.service.graphlcd
    cd script.service.graphlcd
    make

Global installation (for all users on this system):

    git clone https://github.com/M-Reimer/script.service.graphlcd
    cd script.service.graphlcd
    make
    sudo make install

Python 3 support
----------------

Kodi will [migrate to Python 3](https://kodi.wiki/view/General_information_about_migration_to_Python_3). For development and testing purposes, it is already possible to build the graphlcd addon with Python 3 support:

    make PYTHONVERSION=3
