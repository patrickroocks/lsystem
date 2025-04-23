$build_dir = "lsystem_out"
$qt_base_dir = "C:\Qt\6.7.3\mingw_64\bin"
$make = "C:\Qt\Tools\mingw1120_64\bin\mingw32-make"

function main {
	cd ../..
	
	if (-Not (Test-Path -Path "lsystem")) {
		"The directory 'lsystem' was not found. "
		"Make sure that the parent directory of 'build' is named 'lsystem' "
		"and that this script runs in the directory 'lsystem/build'."
		Return
	}
	
	Remove-Item $build_dir -Recurse -ErrorAction Ignore
	mkdir $build_dir
	
	cd $build_dir
	& "$qt_base_dir\qmake" ../lsystem/lsystem.pro -spec win32-g++ CONFIG+=qtquickcompiler
	& "$make" qmake_all
	& "$make" -j8
	cd ..
	"Build done."
	"  Output is in: $pwd\$build_dir"
}

main

cd lsystem/build