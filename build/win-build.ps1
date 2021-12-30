$build_dir = "lsystem_out"
$qt_base_dir = "C:\Qt\5.15.2\mingw81_64\bin"
$make = "C:\Qt\Tools\mingw810_64\bin\mingw32-make"

function main {
	cd ..
	
	if (-Not (Test-Path -Path "../lsystem")) {
		"The directoy 'lsystem' was not found."
		"Make sure that the parent directory of 'build' is named 'lsystem' "
		"and that this script is run in the directory 'lsystem/build'"
		Return
	}
	
	Remove-Item ../$build_dir -Recurse -ErrorAction Ignore
	mkdir ../$build_dir
	
	echo $qt_base_dir\qmake
	cd ../$build_dir
	& "$qt_base_dir\qmake" ../lsystem/lsystem.pro -spec win32-g++ CONFIG+=qtquickcompiler
	& "$make" qmake_all
	& "$make" -j8
	cd ../lsystem
	"Build done"
}

main

cd build