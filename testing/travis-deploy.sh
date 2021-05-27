#!/bin/bash
# script for sourceforge build deployment via travis
# relies on a separate ssh key for uploading

sshid=../id_travissfbot
if [[ $TRAVIS_OS_NAME == linux ]]; then
  filepath=Linux
else
  echo "Unknown platform, exiting!"
  exit 13
fi

# there are no tags due to shallow clones, so improvise
filei=GemRB-$TRAVIS_BRANCH-$TRAVIS_COMMIT-x86_64.AppImage
fileo=GemRB-$TRAVIS_BRANCH-${TRAVIS_COMMIT:0:10}-x86_64.AppImage
filepath="$filepath/$fileo"
pwd
ls -R
scp -i $sshid $filei \
 gemrb-travisbot@frs.sourceforge.net:/home/frs/project/gemrb/Buildbot\\\ Binaries/$filepath
