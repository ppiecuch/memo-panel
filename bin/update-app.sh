#!/bin/bash

set -e

if [[ $OSTYPE == "linux-gnu"* ]]; then
  if [[ $EUID != 0 ]]; then
    echo "Please run $(basename $0) as root"
    exit 0
  fi
fi

app=memo-panel

if [ -f $app ]; then
  echo "=== Installing app -> /var/app/$app"
  mkdir -p /var/app/$app
  rm -rf /var/app/$app/*
  cp $app ${app}.ini /var/app/$app/
  chown -R root.root /var/app/$app
  if [ -f service/$app.service ]; then
    echo "=== Installing service -> /etc/systemd/system/"
    cp service/$app.service /etc/systemd/system/
  fi
  ls -l /var/app/$app/
else
  echo "$app not found."
fi
