pkgname=kodi-addon-graphlcd
pkgver=$(sed -rn 's/<addon.*version="([0-9.]+)".*/\1/p' addon.xml)
pkgrel=1
pkgdesc="Kodi service addon to interface with graphlcd"
url=""
arch=('x86_64' 'i686')
license=('GPL3')
depends=('python2' 'kodi' 'graphlcd-base')

package() {
  cd "$startdir"
  make DESTDIR=$pkgdir install
}
