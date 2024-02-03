#!/bin/bash -e
###############################################################################
### This script is called by (cd .. && make download-external-libs). It will
### git clone thirdparts needed for this project but does not compile them.
### It replaces git submodules that I dislike.
###############################################################################

source ../.makefile/download-external-libs.sh

### TinyXML2 is a simple, small, efficient, C++ XML parser.
### License: Zlib
cloning leethomason/tinyxml2

### Class wrapping client MQTT (mosquitto lib)
### License: MIT
cloning Lecrapouille/MQTT
