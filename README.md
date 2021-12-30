# lsystem

The **lsystem** application is an interactive fractal generator for Lindenmayer systems (also called ["L-systems", Wikipedia](https://en.wikipedia.org/wiki/L-system)). 

It supports Lindenmayer systems with arbitrary many variables (including helper variables which do not result in painted segments), different angles for left/right rotations and scaling down drawings within the recursive call. The user interface is written in Qt5/QML.

## Download Binaries

**Screenshots and prebuild binaries (Ubuntu and Windows):** https://www.p-roocks.de/wordpress2/lsystem-simulator

## Build and run (Ubuntu)

To build lsystem on Ubuntu, you will first need to [install Qt 5.x, x >= 15](https://www.qt.io/download-open-source). To build and start lystem, run (assuming that Qt was installed to `/opt/Qt/5.15.2`):

    git clone https://github.com/patrickroocks/lsystem lsystem
    cd lsystem
    /opt/Qt/5.15.2/gcc_64/bin/qmake CONFIG+=qtquickcompiler
    make qmake_all
    make -j8
    ./lsystemapp/lsystemapp

Your user-defined configs will be stored in a `config.json` located in the working directory. Additionally there are some unit tests which can be run by `./test/test`.

## Build and run (Windows)

Have a look at the PowerShell Scripts in in `build` directory.

## License

lsystem is licensed under the MIT Licence, see `LICENSE` file for details.

## Bugs, comments, suggestions?

Please open an issue here or [write a mail to me](mailto:mail@p-roocks.de).