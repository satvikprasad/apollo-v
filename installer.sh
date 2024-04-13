#!/bin/bash
hdiutil create -size 64m -fs HFS+ -volname "Vizzy" VizzyRW.dmg
hdiutil attach VizzyRW.dmg
cp -r /Applications/Vizzy.app /Volumes/Vizzy/
ln -s /Applications /Volumes/Vizzy/
hdiutil detach /Volumes/Vizzy
hdiutil convert VizzyRW.dmg -format UDZO -o Vizzy.dmg
rm VizzyRW.dmg
