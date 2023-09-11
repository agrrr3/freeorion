#!/bin/bash
set -e
# part of the command chain

# inspiration from (still core20 based) vlc:
# https://github.com/videolan/vlc/blob/75bca603749d8bfb7048a84ea811cbdb19447596/extras/package/snap/snapcraft.yaml
# https://github.com/videolan/vlc/blob/75bca603749d8bfb7048a84ea811cbdb19447596/extras/package/snap/vlc-snap-wrapper.sh
# and also git@github.com:brlin-tw/snapcrafters-template-plus.git

# AUDIO
# https://bugs.launchpad.net/ubuntu/+source/snapd/+bug/1879229
papath="$XDG_RUNTIME_DIR/../pulse/native"
if [ ! -e "$papath" ]; then
    echo "Cannot not find '$papath'"
    # new snapd supports UID env variables, but we want to support older snap installations as well
    #echo "UID $UID  SNAP_UID $SNAP_UID"
    # lets try if creating the directory is enough
    mkdir -p "$XDG_RUNTIME_DIR"
    if [ ! -e "$papath" ]; then
        exit 1
    fi
fi
# we set the usual suspect as a default server
# but we try the original value first - maybe the user actually wants to set a specific pulse server
# the question is if it is reachable
export PULSE_SERVER="${PULSE_SERVER} unix:$papath"

#echo "Found PULSE_SERVER=${PULSE_SERVER}"
# carry on in the command chain:
exec "${@}"
