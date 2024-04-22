#!/bin/bash

BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m'

NAME="Apollo"

if [ ! -e ./${NAME}.dmg ]; then
    echo -e ${BLUE}${NAME}.dmg not found, creating ${NAME}.dmg...${NC}
    ./macos/package.sh
fi

echo -e ${BLUE}Copying ${NAME}.dmg...${NC}
cp ./${NAME}.dmg ../lynx-backend/downloads/osx/${NAME}-latest.dmg

echo -e ${BLUE}Pushing to GitHub...${NC}
pushd ../lynx-backend > /dev/null
git add downloads/osx/${NAME}-latest.dmg
git commit -m "Update ${NAME}.dmg on $(date)"
git push
popd > /dev/null

echo -e ${GREEN}Done.${NC}
