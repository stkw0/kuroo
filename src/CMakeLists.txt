include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#Singletons in common.h depend on lots of things in subdirs
include_directories(
${CMAKE_SOURCE_DIR}/src
${CMAKE_SOURCE_DIR}/src/config
${CMAKE_SOURCE_DIR}/src/core
${CMAKE_SOURCE_DIR}/src/history
${CMAKE_SOURCE_DIR}/src/intro
${CMAKE_SOURCE_DIR}/src/logs
${CMAKE_SOURCE_DIR}/src/portage
${CMAKE_SOURCE_DIR}/src/queue
#include ui_*base.h
${CMAKE_BINARY_DIR}/src
${CMAKE_BINARY_DIR}/src/config
${CMAKE_BINARY_DIR}/src/core
${CMAKE_BINARY_DIR}/src/history
${CMAKE_BINARY_DIR}/src/intro
${CMAKE_BINARY_DIR}/src/logs
${CMAKE_BINARY_DIR}/src/portage
${CMAKE_BINARY_DIR}/src/queue
)

#core
set(core_sources
	core/cacheportagejob.cpp
	core/categorieslistview.cpp
	core/dependatom.cpp
	core/dependencyview.cpp
	core/emerge.cpp
	core/etcupdate.cpp
	core/filewatcher.cpp
	core/global.cpp
	core/packageinspector.cpp
	core/packagelistmodel.cpp
	core/packagelistitem.cpp
	core/packageversion.cpp
	core/portagedb.cpp
	core/portagefiles.cpp
	core/scanhistoryjob.cpp
	core/scanportagejob.cpp
	core/scanupdatesjob.cpp
	core/signalist.cpp
	core/tooltip.cpp
	core/versionview.cpp )
ki18n_wrap_ui( core_sources core/inspectorbase.ui )

#history
set(history_sources
	history/history.cpp
	history/historylistview.cpp
	history/historytab.cpp
	history/mergelistview.cpp
	history/mergetab.cpp )
ki18n_wrap_ui( history_sources history/historybase.ui history/mergebase.ui )

#portage
set(portage_sources
	portage/portage.cpp
	portage/portagetab.cpp
	portage/uninstallinspector.cpp
	portage/portagelistview.cpp )
ki18n_wrap_ui( portage_sources portage/uninstallbase.ui portage/portagebase.ui )

#config
set( config_sources config/configdialog.cpp )
ki18n_wrap_ui(	config_sources
				config/options1.ui
				config/options2.ui
				config/options3.ui
				config/options4.ui )
kconfig_add_kcfg_files( config_sources config/settings.kcfgc )

#queue
set(queue_sources
	queue/queue.cpp
	queue/queuetab.cpp
	queue/queuelistmodel.cpp
	queue/queuelistview.cpp
	queue/queuelistitem.cpp
	queue/queuelistdelegate.cpp )
ki18n_wrap_ui( queue_sources queue/queuebase.ui )

#intro
set( intro_sources intro/introdlg.cpp )
ki18n_wrap_ui( intro_sources intro/intro.ui )

#logs
set( logs_sources logs/log.cpp logs/logstab.cpp )
ki18n_wrap_ui( logs_sources logs/logsbase.ui )

#kuroo
set( kuroo_sources
	kuroo.cpp
	kurooinit.cpp
	kurooview.cpp
	main.cpp
	message.cpp
	statusbar.cpp
	systemtray.cpp )
ki18n_wrap_ui( kuroo_sources messagebase.ui )

add_executable(kuroo
				${core_sources}
				${history_sources}
				${portage_sources}
				${config_sources}
				${queue_sources}
				${intro_sources}
				${logs_sources}
				${kuroo_sources} )

kde_enable_exceptions()

target_link_libraries( kuroo
	Qt5::Core
	KF5::ConfigCore
	KF5::ConfigGui
	KF5::CoreAddons
	KF5::I18n
	KF5::KIOCore
	KF5::ThreadWeaver
	KF5::WidgetsAddons
	KF5::WindowSystem
	KF5::XmlGui
	KF5::KDELibs4Support
	sqlite3
)

########### install ###############
install( TARGETS kuroo ${INSTALL_TARGETS_DEFAULT_ARGS} )
#install( FILES  DESTINATION ${DATA_INSTALL_DIR}/autostart )
install( FILES  config/kurooconfig.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES  kurooui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kuroo )
install( FILES  kuroo.desktop DESTINATION ${XDG_APPS_INSTALL_DIR}/System )
