#!/bin/bash

set -e

export MACOSX_DEPLOYMENT_TARGET=12.0
INSTALLDIR="$HOME/deps"
NPROCS="$(getconf _NPROCESSORS_ONLN)"
SDL=SDL2-2.30.9
QT=6.8.3   # PrimeHack 1.0.9 needs Qt >= 6.3; 6.5.x does not build with the Xcode 16 libc++ on macos-15
QT_SUFFIX=

mkdir deps-build
cd deps-build

export PKG_CONFIG_PATH="$INSTALLDIR/lib/pkgconfig:$PKG_CONFIG_PATH"
export LDFLAGS="-L$INSTALLDIR/lib -dead_strip $LDFLAGS"
export CFLAGS="-I$INSTALLDIR/include -Os $CFLAGS"
export CXXFLAGS="-I$INSTALLDIR/include -Os $CXXFLAGS"

cat > SHASUMS <<EOF
24b574f71c87a763f50704bbb630cbe38298d544a1f890f099a4696b1d6beba4  $SDL.tar.gz
56001b905601bb9023d399f3ba780d7fa940f3e4861e496a7c490331f49e0b80  qtbase-everywhere$QT_SUFFIX-src-$QT.tar.xz
35eb516460f00f264eb504baa253432384351cf23fb9980a5857190e8deef438  qtsvg-everywhere$QT_SUFFIX-src-$QT.tar.xz
02a4e219248b94f1333df843d25763f35251c1074cdc4fb5bda67d340f8c8b3a  qttools-everywhere$QT_SUFFIX-src-$QT.tar.xz
c3c61d79c3d8fe316a20b3617c64673ce5b5519b2e45535f49bee313152fa531  qttranslations-everywhere$QT_SUFFIX-src-$QT.tar.xz
EOF

curl -L \
	-O "https://libsdl.org/release/$SDL.tar.gz" \
	-O "https://download.qt.io/archive/qt/${QT%.*}/$QT/submodules/qtbase-everywhere$QT_SUFFIX-src-$QT.tar.xz" \
	-O "https://download.qt.io/archive/qt/${QT%.*}/$QT/submodules/qtsvg-everywhere$QT_SUFFIX-src-$QT.tar.xz" \
	-O "https://download.qt.io/archive/qt/${QT%.*}/$QT/submodules/qttools-everywhere$QT_SUFFIX-src-$QT.tar.xz" \
	-O "https://download.qt.io/archive/qt/${QT%.*}/$QT/submodules/qttranslations-everywhere$QT_SUFFIX-src-$QT.tar.xz" \

shasum -a 256 --check SHASUMS

echo "Installing SDL..."
tar xf "$SDL.tar.gz"
cd "$SDL"
CC="clang -arch x86_64 -arch arm64" ./configure --prefix "$INSTALLDIR" --without-x
make "-j$NPROCS"
make install
cd ..

echo "Installing Qt Base..."
tar xf "qtbase-everywhere$QT_SUFFIX-src-$QT.tar.xz"
cd "qtbase-everywhere-src-$QT"
# Xcode 16+ SDK math.h no longer defines __MATH_H__, so bundled libpng pulls in the long-gone <fp.h>
sed -i '' 's/#      include <fp.h>/#      include <math.h>/' src/3rdparty/libpng/pngpriv.h
cmake -B build -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DCMAKE_BUILD_TYPE=Release -DFEATURE_optimize_size=ON -DFEATURE_dbus=OFF -DFEATURE_framework=OFF -DFEATURE_icu=OFF -DFEATURE_opengl=OFF -DFEATURE_printsupport=OFF -DFEATURE_sql=OFF -DFEATURE_gssapi=OFF -DFEATURE_system_png=OFF -DFEATURE_system_jpeg=OFF -DCMAKE_MESSAGE_LOG_LEVEL=STATUS
make -C build "-j$NPROCS"
make -C build install
cd ..

echo "Installing Qt SVG..."
tar xf "qtsvg-everywhere$QT_SUFFIX-src-$QT.tar.xz"
cd "qtsvg-everywhere-src-$QT"
cmake -B build -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DCMAKE_BUILD_TYPE=Release
make -C build "-j$NPROCS"
make -C build install
cd ..

echo "Installing Qt Tools..."
tar xf "qttools-everywhere$QT_SUFFIX-src-$QT.tar.xz"
cd "qttools-everywhere-src-$QT"
# Qt 6.8 already skips Linguist (needs PrintSupport, which is disabled), so no patch is needed
cmake -B build -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DCMAKE_BUILD_TYPE=Release -DFEATURE_assistant=OFF -DFEATURE_clang=OFF -DFEATURE_designer=OFF -DFEATURE_kmap2qmap=OFF -DFEATURE_pixeltool=OFF -DFEATURE_pkg_config=OFF -DFEATURE_qev=OFF -DFEATURE_qtattributionsscanner=OFF -DFEATURE_qtdiag=OFF -DFEATURE_qtplugininfo=OFF
make -C build "-j$NPROCS"
make -C build install
cd ..

echo "Installing Qt Translations..."
tar xf "qttranslations-everywhere$QT_SUFFIX-src-$QT.tar.xz"
cd "qttranslations-everywhere-src-$QT"
cmake -B build -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DCMAKE_BUILD_TYPE=Release
make -C build "-j$NPROCS"
make -C build install
cd ..

echo "Cleaning up..."
cd ..
rm -r deps-build
