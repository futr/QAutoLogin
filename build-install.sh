#!/bin/bash -xve

QTVERSION=5
EXECFILE=QAutoLogin
DESKTOP=qautologin.desktop
APPICON=appicon.ico
BINDIR="/opt/qautologin"

qtchooser -run-tool=qmake -qt=$QTVERSION
make

if [ ! -d $BINDIR ]
then
  sudo mkdir $BINDIR
fi

sudo cp $EXECFILE $BINDIR
sudo cp $APPICON $BINDIR/$APPICON
cp $DESKTOP ~/.local/share/applications
make distclean

