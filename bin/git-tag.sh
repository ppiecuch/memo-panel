#!/bin/bash



if [ "$#" -ne 1 ]
then
	echo ""
	echo "Usage: $0 <new version>"
	echo "e.g.: git-tag.sh 3.0"
	echo ""
	echo "Previous version:" 
	git tag  -l '[0-9].*'|sort -V|tail -1
	exit 1
fi

newtag="$1"

if [[ $newtag =~ [0-9]+[.][0-9]+$ ]] ; then
	 echo "New version: $newtag" 
else
	echo "Invalid version: $newtag, aborting"
	exit 1
fi

if [ `git tag|grep $newtag` ] ; then
	echo "$newtag already exists, aborting"
	exit 2
fi

GITTOP=`git rev-parse --show-toplevel`

#echo "Updating version in setup.py and netconf"
#sed -i "s/version =.*/version = '${newtag}'/"  ${GITTOP}/setup.py
#sed -i "s/VERSION =.*/VERSION = '${newtag}'/"  ${GITTOP}/netconf

#git commit -m "bumped netconf version strings to ${newtag}" netconf
#git commit -m "bumped setup.py version strings to ${newtag}" setup.py

lastdate=`git log -1 --format=%ci`


if [ `git status --short|grep -v '^??'|wc -l` -ne 0 ] ; then
	git status --short
	echo "It seems there are not commited changes"
    read -p "Press [Enter] to continue or Ctrl+C to abort: "
fi


git push 
git tag -a $newtag -m "Version ${newtag}"
git push --tags

