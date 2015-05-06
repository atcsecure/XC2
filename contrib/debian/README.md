
Debian
====================
This directory contains files used to package XCurrencyd/XCurrency-qt
for Debian-based Linux systems. If you compile XCurrencyd/XCurrency-qt yourself, there are some useful files here.

## XCurrency: URI support ##


xc-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install xc-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your XCurrency-qt binary to `/usr/bin`
and the `../../share/pixmaps/XCurrency128.png` to `/usr/share/pixmaps`

XCurrency-qt.protocol (KDE)

