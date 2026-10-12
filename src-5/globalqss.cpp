/**
 * Created M/25/11/2025
 * Updated D/21/06/2026
 *
 * Copyright 2025-2026 | Fabrice Creuzot (luigifab) <code~luigifab~fr>
 * https://github.com/luigifab/globalqss
 * https://www.luigifab.fr/gtkqt/globalqss
 *
 * This program is free software, you can redistribute it or modify
 * it under the terms of the GNU General Public License (GPL) as published
 * by the free software foundation, either version 2 of the license, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty, without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the
 * GNU General Public License (GPL) for more details.
 */

#include "globalqss.h"

// can be defined from command line: GQSS_DEBUG GQSS_THEME GQSS_BASE (base style, default Fusion)
//      can be defined from program: GQSS_THEME GQSS_RELOAD
//      can be checked from program: GQSS_SET=yes (plugin loaded) / GQSS_READY=yes (plugin loaded and theme applied) / GQSS_THEME (theme name)
//                                   GQSS_SIGNAL=yes (on desktop theme change)
//
//   engine is loaded without theme: GQSS_SET="yes" + GQSS_THEME=""
// engine is loaded with None theme: GQSS_SET="yes" + GQSS_READY="yes" + GQSS_THEME="None" (empty style sheet)
//      engine is loaded with theme: GQSS_SET="yes" + GQSS_READY="yes" + GQSS_THEME="abc"  (theme files and qt.qss)

GlobalQSS::GlobalQSS() : QProxyStyle(createBaseStyle()) {
}

// without an explicit base, QProxyStyle uses the desktop style, which under Plasma is
// the configured widgetStyle: when that is GlobalQSS itself, it proxies itself forever
QStyle *GlobalQSS::createBaseStyle() {

	QString key = qEnvironmentVariable("GQSS_BASE").trimmed();
	if (key.isEmpty() || (key.compare("GlobalQSS", Qt::CaseInsensitive) == 0))
		key = "Fusion";

	QStyle *style = QStyleFactory::create(key);
	if (!style)
		style = QStyleFactory::create("Fusion");

	if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
		qDebug() << "GQSS: base style" << (style ? style->objectName() : QString("none"));

	return style;
}

GlobalQSS::~GlobalQSS() {

	qunsetenv("GQSS_SET");
	qunsetenv("GQSS_READY");

	#ifdef Q_OS_LINUX
		if (gqss_monitor)
			QDBusConnection::sessionBus().disconnect("ca.desrt.dconf", "/ca/desrt/dconf/Writer/user", "ca.desrt.dconf.Writer", "Notify", this, SLOT(onNotify(QString)));
	#endif
}

bool GlobalQSS::event(QEvent *e) {
	//qDebug() << "GQSS: event type:" << e->type();
	// @see https://stackoverflow.com/q/79928058
	return (e->type() == QEvent::MetaCall) ? QObject::event(e) : QProxyStyle::event(e);
}

void GlobalQSS::onNotify(QString path) {

	if (qApp && ((path == "/org/gnome/desktop/interface/gtk-theme") || (path == "/org/mate/desktop/interface/gtk-theme"))) {

		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS: themeChanged";

		qunsetenv("GQSS_THEME");
		qputenv("GQSS_SIGNAL", "yes");

		gqss_applied = false;
		polish(qApp); // apply theme

		qunsetenv("GQSS_SIGNAL");
	}
}

void GlobalQSS::polish(QApplication *app) {

	if (qEnvironmentVariableIsSet("GQSS_RELOAD")) {
		qunsetenv("GQSS_RELOAD");
		gqss_applied = false;
		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS: reload";
	}
	else if (gqss_applied) {
		return; // yolo
	}

	// @see https://github.com/loot/loot/issues/1896
	// start monitoring of desktop theme change with dbus (disabled when application is started with GQSS_THEME=xyz)
	// QDBusConnectionPrivate() got message (signal): QDBusMessage(type=Signal, service=":1.10", path="/ca/desrt/dconf/Writer/user", interface="ca.desrt.dconf.Writer", member="Notify", signature="sass", contents=("/org/mate/desktop/interface/gtk-theme", {""}, ":1.10:user:xyz") )
	#ifdef Q_OS_LINUX
		if (!gqss_monitor && !qEnvironmentVariableIsSet("GQSS_SET") && !qEnvironmentVariableIsSet("GQSS_THEME")) {

			gqss_monitor = QDBusConnection::sessionBus().connect("ca.desrt.dconf", "/ca/desrt/dconf/Writer/user", "ca.desrt.dconf.Writer", "Notify", this, SLOT(onNotify(QString)));

			if (qEnvironmentVariableIsSet("GQSS_DEBUG")) {
				if (gqss_monitor)
					qDebug() << "GQSS: monitor started";
				else
					qDebug() << "GQSS: monitor error" << QDBusConnection::sessionBus().lastError().message();
			}
		}
	#endif

	qputenv("GQSS_SET", "yes");

	// read theme from GQSS_THEME or from MATE or from GNOME
	// for MATE/GNOME, if theme found, set GQSS_THEME
	QString themeName = qEnvironmentVariable("GQSS_THEME").trimmed();
	if (themeName.isEmpty())
		themeName = readGtkSettingsThemeName();
	if (themeName.isEmpty())
		themeName = readThemeName("org.mate.interface", "gtk-theme");
	if (themeName.isEmpty())
		themeName = readThemeName("org.gnome.desktop.interface", "gtk-theme");

	// load and apply theme
	// if found and applied, set GQSS_READY
	// else, unset GQSS_READY
	if (themeName.isEmpty()) {
		qunsetenv("GQSS_READY");
	}
	else {
		QString css, qtVersion = QString::number(QT_VERSION_MAJOR);
		bool found = (themeName == "None");

		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS: theme" << themeName;

		// do nothing (None theme) or load theme (theme files and qt.qss)
		if (!found) {

			// load from theme files
			QStringList themePaths = {
				QDir::homePath() + "/.themes/" + themeName + "/qt" + qtVersion,
				QDir::homePath() + "/.local/share/themes/" + themeName + "/qt" + qtVersion,
				"/usr/local/share/themes/" + themeName + "/qt" + qtVersion,
				"/usr/share/themes/" + themeName + "/qt" + qtVersion,
			};

			#ifdef Q_OS_WIN
				themePaths << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../share/themes/" + themeName + "/qt" + qtVersion);
			#endif

			for (const QString &path : themePaths) {

				QDir dir(path);
				if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
					qDebug() << "GQSS:   dir" << path;

				if (dir.exists()) {

					dir.setFilter(QDir::Files);
					dir.setNameFilters(QStringList() << "*.qss");

					QStringList files = dir.entryList();
					if (!files.isEmpty()) {

						std::sort(files.begin(), files.end(), [](const QString &a, const QString &b) {
							return QString(a).replace('-', '~').compare(QString(b).replace('-', '~'), Qt::CaseInsensitive) < 0;
						});

						for (const QString &fileName : files)
							css += readFile(app, dir, fileName);

						found = true;
						break;
					}
				}
			}

			// load from qt.qss
			for (const QString &path : {
				QDir::homePath() + "/.config/qt" + qtVersion
			}) {

				QDir dir(path);
				if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
					qDebug() << "GQSS:   dir" << path;

				if (dir.exists()) {
					css += readFile(app, dir, "qt.qss");
					css += readFile(app, dir, "qt-rtl.qss");
					break;
				}
			}
		}

		// apply QSS
		if (found) {

			gqss_applied = true;

			qputenv("GQSS_READY", "yes");
			if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
				qDebug() << "GQSS: setStyleSheet";

			//app->setStyleSheet(css); // vlc = crash
			if (QCoreApplication::applicationName().toLower().contains("vlc"))
				QMetaObject::invokeMethod(app, "setStyleSheet", Qt::QueuedConnection, Q_ARG(QString, css));
			else
				QMetaObject::invokeMethod(app, "setStyleSheet", Qt::DirectConnection, Q_ARG(QString, css));
		}
		else {
			qunsetenv("GQSS_READY");
		}
	}

	if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
		qDebug() << "GQSS: (end)";
}

QString GlobalQSS::readThemeName(QString path, QString key) {

	QString result;

	#ifdef Q_OS_LINUX
		QProcess cmd;

		cmd.start("gsettings", QStringList() << "get" << path << key);
		cmd.waitForFinished(200);
		result = QString::fromUtf8(cmd.readAllStandardOutput()).trimmed();
		cmd.close();

		if (!result.isEmpty()) {
			if (result.startsWith("'") && result.endsWith("'"))
				result = result.mid(1, result.length() - 2);
			if (!result.isEmpty())
				qputenv("GQSS_THEME", result.toUtf8());
		}
	#endif

	return result;
}

// Plasma writes the GTK theme to gtk-3.0/settings.ini and may leave gsettings on another theme
QString GlobalQSS::readGtkSettingsThemeName() {

	QString result;

	if (!qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("KDE", Qt::CaseInsensitive))
		return result;

	QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/gtk-3.0/settings.ini";
	if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
		qDebug() << "GQSS:  gtk settings" << path;

	QSettings settings(path, QSettings::IniFormat);
	result = settings.value("Settings/gtk-theme-name").toString().trimmed();
	if (!result.isEmpty())
		qputenv("GQSS_THEME", result.toUtf8());

	return result;
}

QString GlobalQSS::readFile(QApplication *app, QDir dir, QString name) {

	QString result, path = dir.absoluteFilePath(name);
	if (name.endsWith("-rtl.qss") && (app->layoutDirection() != Qt::RightToLeft))
		return result;

	QFile file(path);
	if (file.open(QFile::ReadOnly)) {

		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS:  file" << path;

		result = QString::fromUtf8(file.readAll()).trimmed().replace("url(\"", "url(\"" + dir.absolutePath() + "/") + "\n";
		file.close();
	}

	return result;
}