/**
 * Created M/25/11/2025
 * Updated D/11/10/2026
 *
 * Copyright 2025-2027 | Fabrice Creuzot (luigifab) <code~luigifab~fr>
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

// read only environment variables, can be defined from command line: GQSS_DEBUG GQSS_THEME
//
// qApp properties
//  can be defined from program: GQSS_THEME GQSS_RELOAD
//  can be checked from program: GQSS_SET=true (plugin loaded) / GQSS_READY=true (plugin loaded and theme applied) / GQSS_THEME (theme name)
//                               GQSS_SIGNAL=true (on desktop theme change)
//
//   engine is loaded without theme: GQSS_SET=true + GQSS_THEME=""
// engine is loaded with stylesheet: GQSS_SET=true + GQSS_THEME="-stylesheet"
// engine is loaded with None theme: GQSS_SET=true + GQSS_READY=true + GQSS_THEME="None" (empty style sheet)
//      engine is loaded with theme: GQSS_SET=true + GQSS_READY=true + GQSS_THEME="abc"  (theme files and qt.qss)

GlobalQSS::~GlobalQSS() {

	if (qApp && !QCoreApplication::closingDown()) {
		qApp->setProperty("GQSS_SET", QVariant());
		qApp->setProperty("GQSS_READY", QVariant());
	}

	#ifdef Q_OS_LINUX
		if (gqss_monitor)
			QDBusConnection::sessionBus().disconnect("ca.desrt.dconf", "/ca/desrt/dconf/Writer/user", "ca.desrt.dconf.Writer", "Notify", this, SLOT(onNotify(QString)));
	#endif
}

void GlobalQSS::onNotify(QString path) {

	if (qApp && ((path == "/org/gnome/desktop/interface/gtk-theme") || (path == "/org/mate/desktop/interface/gtk-theme"))) {

		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS: themeChanged";

		qApp->setProperty("GQSS_THEME", QVariant());
		qApp->setProperty("GQSS_SIGNAL", true);

		gqss_applied = false;
		polish(qApp); // apply theme

		qApp->setProperty("GQSS_SIGNAL", QVariant());
	}
}

void GlobalQSS::polish(QApplication *app) {

	bool reload = app->property("GQSS_RELOAD").toBool();
	if (reload) {
		app->setProperty("GQSS_RELOAD", QVariant());
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
		if (!gqss_monitor && !app->property("GQSS_SET").toBool() && !qEnvironmentVariableIsSet("GQSS_THEME") && (geteuid() != 0)) {

			gqss_monitor = QDBusConnection::sessionBus().connect("ca.desrt.dconf", "/ca/desrt/dconf/Writer/user", "ca.desrt.dconf.Writer", "Notify", this, SLOT(onNotify(QString)));

			if (qEnvironmentVariableIsSet("GQSS_DEBUG")) {
				if (gqss_monitor)
					qDebug() << "GQSS: monitor started";
				else
					qDebug() << "GQSS: monitor error" << QDBusConnection::sessionBus().lastError().message();
			}
		}
	#endif

	app->setProperty("GQSS_SET", true);

	// do not apply theme when -stylesheet is used
	if (!reload && app->styleSheet().startsWith("file:///")) {
		QFileInfo sheetFile(app->styleSheet().mid(8));
		if (sheetFile.isFile() && sheetFile.isReadable()) {
			app->setProperty("GQSS_THEME", "-stylesheet");
			app->setProperty("GQSS_READY", QVariant());
			original_sheet = app->styleSheet();
			if (qEnvironmentVariableIsSet("GQSS_DEBUG")) {
				qDebug() << "GQSS: -stylesheet" << original_sheet;
				qDebug() << "GQSS: (end)";
			}
			return;
		}
	}

	// read theme from GQSS_THEME or from MATE or from GNOME
	// if theme found, set GQSS_THEME
	QString themeName = app->property("GQSS_THEME").toString().trimmed();
	if (themeName.isEmpty())
		themeName = qEnvironmentVariable("GQSS_THEME").trimmed();
	if (themeName.isEmpty())
		themeName = readThemeName("org.mate.interface", "gtk-theme");
	if (themeName.isEmpty())
		themeName = readThemeName("org.gnome.desktop.interface", "gtk-theme");
	if ((themeName == ".") || (themeName == "..") || themeName.contains('/') || themeName.contains('\\'))
		themeName.clear();
	if (!themeName.isEmpty())
		app->setProperty("GQSS_THEME", themeName);

	// load and apply theme
	// if found and applied, set GQSS_READY
	// else, unset GQSS_READY
	if (themeName.isEmpty()) {
		app->setProperty("GQSS_READY", QVariant());
	}
	else if (themeName == "-stylesheet") {

		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS: setStyleSheet";

		//app->setStyleSheet(css); // vlc = crash
		if (QCoreApplication::applicationName().toLower().contains("vlc"))
			QMetaObject::invokeMethod(app, "setStyleSheet", Qt::QueuedConnection, Q_ARG(QString, original_sheet));
		else
			QMetaObject::invokeMethod(app, "setStyleSheet", Qt::DirectConnection, Q_ARG(QString, original_sheet));

		app->setProperty("GQSS_READY", QVariant());
	}
	else {
		QString css, qtVersion = QString::number(QT_VERSION_MAJOR);
		bool found = (themeName == "None");

		if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
			qDebug() << "GQSS: theme" << themeName;

		// do nothing (None theme) or load theme (theme files *.qss and qt.qss)
		if (!found) {

			QStringList themePaths;
			QString userDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

			// list available user themes (<home>/.local/share/themes && <home>/.themes)
			themePaths << QDir(userDir).filePath("themes/" + themeName + "/qt" + qtVersion);
			themePaths << QDir::home().filePath(".themes/" + themeName + "/qt" + qtVersion);

			// list available system themes (/usr/local/share/themes && /usr/share/themes)
			for (QString dir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
				if (dir != userDir)
					themePaths << QDir(dir).filePath("themes/" + themeName + "/qt" + qtVersion);
			}

			#ifdef Q_OS_WIN
				themePaths << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../share/themes/" + themeName + "/qt" + qtVersion);
			#endif

			// load from theme files
			for (const QString &path : themePaths) {

				QDir dir(path);
				if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
					qDebug() << "GQSS:   dir" << path;

				if (dir.exists("qt.qss")) {

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
				QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/qt" + qtVersion
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

			app->setProperty("GQSS_READY", true);
			if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
				qDebug() << "GQSS: setStyleSheet";

			//app->setStyleSheet(css); // vlc = crash
			if (QCoreApplication::applicationName().toLower().contains("vlc"))
				QMetaObject::invokeMethod(app, "setStyleSheet", Qt::QueuedConnection, Q_ARG(QString, css));
			else
				QMetaObject::invokeMethod(app, "setStyleSheet", Qt::DirectConnection, Q_ARG(QString, css));
		}
		else {
			app->setProperty("GQSS_READY", QVariant());
		}
	}

	if (qEnvironmentVariableIsSet("GQSS_DEBUG"))
		qDebug() << "GQSS: (end)";
}

QString GlobalQSS::readThemeName(QString path, QString key) {

	QString result;

	#ifdef Q_OS_LINUX
		// root user
		if (geteuid() == 0) {
			bool ok = false;
			uid_t uid = (uid_t)-1;
			QFile f("/proc/self/loginuid");
			if (f.open(QFile::ReadOnly)) {
				uid = f.readAll().trimmed().toUInt(&ok);
				f.close();
			}
			if (ok && (uid != 0) && (uid != (uid_t)-1)) {
				struct passwd *pw = getpwuid(uid);
				if (pw && pw->pw_dir) {
					QProcessEnvironment env;
					env.insert("HOME", QString::fromUtf8(pw->pw_dir));
					env.insert("XDG_DATA_DIRS", qEnvironmentVariable("XDG_DATA_DIRS"));
					QProcess cmd;
					cmd.setProcessEnvironment(env);
					cmd.start("/usr/bin/setpriv", QStringList()
						<< ("--reuid=" + QString::number(uid))
						<< ("--regid=" + QString::number(pw->pw_gid))
						<< "--clear-groups"
						<< "--no-new-privs"
						<< "/usr/bin/gsettings" << "get" << path << key);
					cmd.waitForFinished(200);
					result = QString::fromUtf8(cmd.readAllStandardOutput()).trimmed();
					cmd.close();
				}
				else {
					return result;
				}
			}
			else {
				return result;
			}
		}
		// normal user
		else {
			QProcess cmd;
			cmd.start("gsettings", QStringList() << "get" << path << key);
			cmd.waitForFinished(200);
			result = QString::fromUtf8(cmd.readAllStandardOutput()).trimmed();
			cmd.close();
		}
	#endif

	if (!result.isEmpty()) {
		if (result.startsWith("'") && result.endsWith("'"))
			result = result.mid(1, result.length() - 2);
		if ((result == ".") || (result == "..") || result.contains('/') || result.contains('\\'))
			result.clear();
	}

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