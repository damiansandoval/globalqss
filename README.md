-- NIGHTLY --

# Global QSS

This engine allows theming of Qt applications using QSS files, similar to how GTK applications are themed with CSS files.

It finds the theme name from the environment variable `GQSS_THEME`, or from MATE settings, or from GNOME settings. It supports theme reload on desktop theme change (via DBus). It does nothing when -stylesheet is used.

## Tips

Standard theme files are:
```
.themes/<example>/qt5/qt.qss
.themes/<example>/qt5/qt-rtl.qss
.themes/<example>/qt5/*.qss
...
.themes/<example>/qt6/qt.qss
.themes/<example>/qt6/qt-rtl.qss
.themes/<example>/qt6/*.qss
```

Just as CSS for GTK isn't really CSS, QSS for Qt isn't really CSS either, and it supports even fewer features, [read the docs](https://doc.qt.io/qt-6/stylesheet-reference.html).

- Use `image: url("img/menu-checkbox-checked.svg");` for theme images
- Use `image: url('/usr/share/icons/Adwaita/symbolic/ui/pan-down-symbolic.svg');` for system images

For an example, see [awf-qt](https://github.com/luigifab/awf-extended) and [human-theme](https://github.com/luigifab/human-theme).

## RTL

Use `*-rtl.qss` for file names.

Starting from version 1.2.0, RTL files are loaded after the non-rtl files only when the Qt application is running in right-to-left mode. Previously, these files were always loaded but in reverse order, so if both files define the same property (for example `QWidget { margin }`), the non-rtl file overrides and the theme is ok.

## Installation

It requires **Qt 5.10+** *(including 5.15)* or **Qt 6.0+** *(including 6.12)*.

> The road is long, but the way is free — we always have a choice.

#### Install via package manager

* `sudo apt install qt5-style-globalqss qt6-style-globalqss`
* `sudo dnf install qt5-globalqss qt6-globalqss`

Replace `apt`/`dnf` with your package manager, or use a graphical interface.

#### Build from source

* To compile with Qt 5 & 6 run: `build.sh`
* To compile and create DEB packages for Debian & Ubuntu run: `scripts/debian/deb.sh`
* To compile and create RPM packages for Fedora run: `scripts/fedora/rpm.sh`

#### Install via PPA

```bash
# https://launchpad.net/~luigifab/+archive/ubuntu/packages

sudo add-apt-repository ppa:luigifab/packages
sudo apt update
sudo apt install qt5-style-globalqss qt6-style-globalqss
# or
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys FFE5BD439356DF7D
echo "deb http://ppa.launchpad.net/luigifab/packages/ubuntu focal main" | sudo tee -a /etc/apt/sources.list
sudo apt update
sudo apt install qt5-style-globalqss qt6-style-globalqss
# or
sudo wget -O /etc/apt/trusted.gpg.d/luigifab.gpg https://www.luigifab.fr/apt.gpg
echo "deb http://ppa.launchpad.net/luigifab/packages/ubuntu focal main" | sudo tee -a /etc/apt/sources.list
sudo apt update
sudo apt install qt5-style-globalqss qt6-style-globalqss
# or
wget -qO - https://www.luigifab.fr/apt.gpg | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/luigifab.gpg
echo "deb http://ppa.launchpad.net/luigifab/packages/ubuntu focal main" | sudo tee -a /etc/apt/sources.list
sudo apt update
sudo apt install qt5-style-globalqss qt6-style-globalqss

# sha256sum /etc/apt/trusted.gpg.d/luigifab.gpg
578c89a677048e38007462d543686b53587efba9f93814601169253c45ff9213
# apt-key list
/etc/apt/trusted.gpg.d/luigifab.gpg
pub   rsa4096 2020-10-31 [SC]
      458B 0C46 D024 FD8C B8BC  99CD FFE5 BD43 9356 DF7D
```

## Dev

You can set `GQSS_DEBUG=1` from command line to enable debug mode.\
When the plugin has applied the theme, the `GQSS_READY` property of `qApp` is `true`.

From any program, you can reload the theme with:
```c++
	if (qApp->property("GQSS_SET").toBool()) {
		//qApp->setProperty("GQSS_THEME", "[newThemeName]");
		qApp->setProperty("GQSS_RELOAD", true);
		QApplication::style()->polish(qApp);
		QApplication::processEvents();
		window->resize(window->minimumSizeHint());
	}
```

#### VLC

The customize interface dialog can crash the program.

#### OpenShot

Stay on version 3.1.1 to use the system theme.

Don't forget to add `::1 openshot.org www.openshot.org` in _/etc/hosts_ to disable the OpenShot mouchard.

## Copyright

- Current version: 2.0.0~nightly20261011
- Compatibility: Qt 5.10..5.15 / 6.0..6.12
- Links: [luigifab.fr](https://www.luigifab.fr/gtkqt/globalqss) - [github.com](https://github.com/luigifab/globalqss) - [ppa/dpa](https://launchpad.net/~luigifab/+archive/ubuntu/packages)

This program is provided under the terms of the **GNU GPLv2+** license.\
If you like, take some of your time to improve the translations, go to https://bit.ly/2HyCCEc.
