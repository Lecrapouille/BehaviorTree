##=====================================================================
## https://github.com/Lecrapouille/BehaviorTree
## BehaviorTree: Library for Behavior Tree
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
## along with Highway.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := Library for Behavior Tree
include $(M)/project/Makefile

###################################################
# External dependencies
#
PKG_LIBS += yaml-cpp tinyxml2

###################################################
# Behavior Tree Library
#
LIB_FILES := $(call rwildcard,$(P)/src,*.cpp)
INCLUDES += $(P) $(P)/src $(P)/include

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
all:: viewer demos

###################################################
# Behavior Tree Viewer
#
.PHONY: viewer
viewer:
	@$(call print-from,"Compiling viewer",$(PROJECT_NAME),$(P)/viewer)
	$(MAKE) -C $(P)/viewer all

###################################################
# Compile demos
#
DEMOS = $(sort $(dir $(wildcard ./docs/demos/*/.)))
.PHONY: demos
demos:
	@$(call print-from,"Compiling demos",$(PROJECT_NAME),$(DEMOS))
	@mkdir -p $(BUILD_DIR)/demos
	@for i in $(DEMOS);            \
	do                             \
		$(MAKE) -C $$i all;        \
	done;