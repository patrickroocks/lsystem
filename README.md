# lsystem

The **lsystem** application is an interactive fractal generator for Lindenmayer systems (also called ["L-systems", Wikipedia](https://en.wikipedia.org/wiki/L-system)). 

It supports Lindenmayer systems with arbitrary many variables (including helper variables which do not result in painted segments), different angles for left/right rotations and scaling down drawings within the recursive call. The user interface is written in Qt5/QML.

## Download Binaries

**Screenshots and prebuild binaries (Ubuntu and Windows):** https://www.p-roocks.de/wordpress2/lsystem-simulator

## Prerequisites to build

To build lsystem on Ubuntu, you will first need to [install Qt >= 6.5](https://www.qt.io/download-open-source). Moreover you will need to install:

	sudo apt install libxkbcommon-dev libxcb-xinerama0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxkbcommon-x11-0 libxcb-cursor0

## Build and run (Ubuntu)

 To build and start lystem, run (assuming that Qt was installed to `/opt/Qt/6.5`):

    git clone https://github.com/patrickroocks/lsystem lsystem
    cd lsystem
	/opt/Qt/6.5/gcc_64/bin/qmake CONFIG+=qtquickcompiler
    make qmake_all
    make -j8
    ./lsystemapp/lsystemapp

Your user-defined configs will be stored in a `config.json` located in the working directory. Additionally there are some unit tests which can be run by `./test/test`.

## Developing within QtCreater and Qt Design Studio

* The lystem project in QT creator
	- First configure the project (`lystem.pro`) using Qt 6.5 or higher.
	- Add the following custom build step in order to get the newest QML files from the Qt Design Studio project:
		- Command: `bash`
		- Arguments: `lsystemapp/util/qml/LsystemQml/copy_qml.sh`
		- Working Directory: `%{sourceDir}`
* The QML project in Qt Design Studio
	- run design studio, typicall installed in `/opt/Qt/Tools/QtDesignStudio/bin/qtdesignstudio`
	- open `lsystemapp/util/qml/LsystemQml/LsystemQml.qmlproject

## Build and run (Windows)

Have a look at the PowerShell Scripts in in `build` directory.

## License

lsystem is licensed under the MIT Licence, see `LICENSE` file for details.

## Bugs, comments, suggestions?

Please open an issue here or [write a mail to me](mailto:mail@p-roocks.de).
