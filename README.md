# lsystem

The **lsystem** app is an interactive fractal generator for [Lindenmayer systems (also called "L-systems")](https://en.wikipedia.org/wiki/L-system). 

It supports Lindenmayer systems with arbitrary many variables (including helper variables which do not result in painted segments), different angles for left/right rotations and scaling down drawings within the recursive call. The user interface is written in Qt5.

## Build and run

To build lsystem, you will need to [install the Qt Library](https://wiki.qt.io/Install_Qt_5_on_Ubuntu). To build and start lystem, run:

    git clone https://github.com/patrickroocks/lsystem lsystem
    cd lsystem
    qmake
    make
    ./lsystemapp/lsystemapp

Your user-defined configs will be stored in the json file located in the working directory. Additionally there are some unit tests which can be run by `./test/test`.

## Prebuild binary

On the [lsystem page on my personal web page](https://www.p-roocks.de/wordpress2/lsystem-simulator) I provide some screenshots and a [prebuild binary for Linux](https://www.p-roocks.de/files/lsystemapp). Download it and run 

    chmod +x lsystemapp
    ./lsystemapp
	
to start it. Run `./lsystemapp --version` to check if you have the newest version, i.e., the same version as in the [version file](https://github.com/patrickroocks/lsystem/blob/main/lsystem/lsystemapp/version.h) on this repository. 

## Learn more about Lindenmayer systems

Have a look at [L-System article on Wikipedia](https://en.wikipedia.org/wiki/L-system).

## Bugs, comments, suggestions?

Please open an issue here or [write a mail to me](mailto:mail@p-roocks.de).
