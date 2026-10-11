/**
 * Created M/25/11/2025
 * Updated M/29/09/2026
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

#ifndef GLOBALQSS_H
#define GLOBALQSS_H

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProxyStyle>
#include <QStandardPaths>
#include <QString>
#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <QDBusError>
#include <QProcess>
#include <QProcessEnvironment>
#include <pwd.h>
#include <unistd.h>
#endif

class GlobalQSS : public QProxyStyle
{
	Q_OBJECT
	bool gqss_monitor = false;
	bool gqss_applied = false;
	QString original_sheet;

public:
	GlobalQSS() = default;
	~GlobalQSS();
	bool event(QEvent *e) override;
	void polish(QApplication *app) override;

private:
	QString readThemeName(QString path, QString key);
	QString readFile(QApplication *app, QDir dir, QString name);

private slots:
	void onNotify(QString path);
};

#endif