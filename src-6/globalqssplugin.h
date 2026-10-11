/**
 * Created M/25/11/2025
 * Updated V/13/03/2026
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

#ifndef GLOBALQSSPLUGIN_H
#define GLOBALQSSPLUGIN_H

#include <QStylePlugin>

class GlobalQSSPlugin : public QStylePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "globalqssplugin.json")

public:
	GlobalQSSPlugin(QObject *parent = nullptr);
	QStringList keys() const;
	QStyle *create(const QString &key) override;
};

#endif