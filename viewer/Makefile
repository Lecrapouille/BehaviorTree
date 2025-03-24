##=====================================================================
## BehaviorTree: A library for Behavior Tree.
## Copyright 2025 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of BehaviorTree.
##
## BehaviorTree is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := BehaviorTreeVisualizer
TARGET_DESCRIPTION := Real-time Behavior Tree Viewer
include $(M)/project/Makefile

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/viewer/src

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P) $(P)/viewer/src $(P)/include

###################################################
# External dependencies
#
PKG_LIBS += sfml-graphics
PKG_LIBS += sfml-window
PKG_LIBS += sfml-system
PKG_LIBS += yaml-cpp

###################################################
# Make the list of compiled files for the application
#
SRC_FILES += $(call rwildcard,$(P)/viewer/src,*.cpp)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile