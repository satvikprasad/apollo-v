#!/bin/bash

BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m'

if [ ! -e ./Vizzy.dmg ]; then
    echo -e ${BLUE}Vizzy.dmg not found, creating Vizzy.dmg...${NC}
    make package
fi

echo -e ${BLUE}Copying Vizzy.dmg...${NC}
cp ./Vizzy.dmg ../lynx-backend/downloads/osx/Vizzy-latest.dmg

echo -e ${BLUE}Pushing to GitHub...${NC}
pushd ../lynx-backend > /dev/null
git add downloads/osx/Vizzy-latest.dmg
git commit -m "Update Vizzy.dmg on $(date)"
git push
popd > /dev/null

echo -e ${GREEN}Done.${NC}
