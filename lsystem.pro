!versionAtLeast(QT_VERSION, 6.5.0) {
	message("Cannot use Qt $${QT_VERSION}")
	error("Use Qt 6.5 or newer")
}

TEMPLATE = subdirs

SUBDIRS += \
	utiltestlib \
	test \
	lsystemapp

OTHER_FILES += \
	CHANGES.md
