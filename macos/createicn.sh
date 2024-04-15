#!/bin/bash
export ICONDIR=$2.iconset
export ORIGICON=$1

mkdir $ICONDIR

# Normal screen icons
for SIZE in 16 32 64 128 256 512; do
sips -z $SIZE $SIZE "$ORIGICON" --out $ICONDIR/icon_${SIZE}x${SIZE}.png ;
done

# Retina display icons
for SIZE in 32 64 256 512; do
sips -z $SIZE $SIZE "$ORIGICON" --out $ICONDIR/icon_$(expr $SIZE / 2)x$(expr $SIZE / 2)x2.png ;
done

# Make a multi-resolution Icon
iconutil -c icns -o "$3" $ICONDIR
rm -rf $ICONDIR #it is useless now
