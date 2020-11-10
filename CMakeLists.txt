set( CMAKE_BUILD_TYPE DebugFull )
#Warned by dev build, probably works with older than 3.7
cmake_minimum_required( VERSION 3.7 )
set (QT_MIN_VERSION "5.7.0")
set (KF5_MIN_VERSION "5.15")
#Warned by dev build, apparently having mixed full path and search references
#causes different behavior depending on this policy, we should be fine with NEW
if( COMMAND cmake_policy)
	cmake_policy( SET CMP0003 NEW )
endif( COMMAND cmake_policy )

project( kuroo )

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package( ECM REQUIRED NO_MODULE )
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(FeatureSummary)

include(GenerateExportHeader)
include(ECMInstallIcons)
include(ECMAddTests)

#find_package( Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
#	Core
#	Gui
#	Widgets
#)

#find_package(Qt5Core)
#find_package(Qt5Gui)
find_package(Qt5Widgets REQUIRED)

find_package( KF5 ${KF5_MIN_VERSION} REQUIRED
	Config
	ConfigWidgets
	CoreAddons
	I18n
	IconThemes
	ItemModels
	ItemViews
	KIO
	ThreadWeaver
	Su
	WidgetsAddons
	WindowSystem
	XmlGui
	KDELibs4Support
)

#
# checks for functions and stuff #Randomly borrowed from ktorrent
#
include(CheckIncludeFiles)
include(CheckFunctionExists)

#find_package( KDE4 REQUIRED )
#include( KDE4Defaults )
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)	#from convert-kurl.pl

add_subdirectory( src )
add_subdirectory( po )
add_subdirectory( pics )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
