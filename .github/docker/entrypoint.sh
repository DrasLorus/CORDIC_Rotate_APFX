#!/bin/bash -l

pacman --needed --noconfirm -Syu base-devel cmake git catch2 wget

mkdir /build-hls
chmod 777 /build-hls

pushd /build-hls || exit 1
wget https://github.com/DrasLorus/HLS_arbitrary_Precision_Types/releases/download/v0.0.1.rc2/PKGBUILD
useradd builder
su builder -c makepkg
pacman --noconfirm -U -- *.pkg.tar.zst
popd || exit 1

