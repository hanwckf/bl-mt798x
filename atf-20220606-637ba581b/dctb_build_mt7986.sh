#!/bin/bash
#

#
#The shell script is to collect build images and transfer them out
#Z:\icb_rebb-main\atf\build\mt7986\release

LOAD_NAME="mt7986"

if [ -d mtkbuild ]; then
   echo "Remove old images!"
   rm -rf mtkbuild
fi

mkdir mtkbuild

echo "Collect build images!"

#cp -f ./out/boot.img ./mtkbuild/
#cp -f ./prebuilt/bootable/2701_loader/upstream/* ./mtkbuild/
#cp -f ./prebuilt/bootable/*.gz ./mtkbuild/
cp -rf ./build/$LOAD_NAME/release/bl2.* ./mtkbuild/
#cp -rf ./build/tmp/deploy/images/$LOAD_NAME/*.* ./mtkbuild/

#echo "Make tar package!"
#tar -zcf ./build/$LOAD_NAME.tar.gz ./mtkbuild
echo "mtkbuild transfer tar package!"
mtkbuild -o ./mtkbuild/
