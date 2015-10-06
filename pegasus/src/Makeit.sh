#!/bin/sh
export PEGASUS_ROOT=/usr/src/packages/BUILD/pegasus
#export PEGASUS_ROOT=/data/sandbox/pegasus
export PEGASUS_HOME=$PEGASUS_ROOT/_build
export PEGASUS_PLATFORM="LINUX_IX86_GNU"
export PEGASUS_ENVVAR_FILE=$PEGASUS_ROOT/env_var_Linux.status
export PATH=$PATH:$PEGASUS_HOME/bin
export LD_LIBRARY_PATH=$PEGASUS_HOME/lib
make $1

