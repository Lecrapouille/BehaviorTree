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
TARGET_NAME := $(PROJECT_NAME)-UnitTest
TARGET_DESCRIPTION := Unit tests for $(PROJECT_NAME)
COMPILATION_MODE = test
include $(M)/project/Makefile

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/tests $(P)/src

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P) $(P)/src $(P)/include

###################################################
# Make the list of compiled files for the library
#
SRC_FILES += $(call rwildcard,$(P)/src,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/tests,*.cpp)

###################################################
# Compare results with
# https://github.com/BehaviorTree/BehaviorTree.CPP
#
DEFINES += -DUSE_LIB_BEHAVIOR_TREE_CPP
THIRDPART_LIBS += libbehaviortree_cpp

###################################################
# Set Libraries. For knowing which libraries
# is needed please read the external/README.md file.
#
PKG_LIBS += yaml-cpp
# PKG_LIBS += tinyxml2
PKG_LIBS += gtest gmock

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile