$build_dir = "lsystem_out"
$deploy_dir = "lsystem_win"
$qt_base_dir = "C:\Qt\5.15.2\mingw81_64\bin"
$qt_path_ext = "C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin"
$out_file = "lsystem_win.zip"

function main {
	cd ../..
	if (-Not (Test-Path -Path $build_dir)) {
		"Build directory '../../$build_dir' was not found. Run 'win-build.ps1' first."
		Return
	}
	$env:Path += ";C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin"
	
	Remove-Item $deploy_dir -Recurse -ErrorAction Ignore
	mkdir $deploy_dir
	
	copy "$build_dir\lsystemapp\release\lsystemapp.exe" "$deploy_dir\lsystemapp.exe"
	& "$qt_base_dir\windeployqt" $deploy_dir\lsystemapp.exe --no-virtualkeyboard --no-opengl-sw --no-system-d3d-compiler --qmldir lsystem\lsystemapp\util\qml
	Remove-Item "$deploy_dir/qmltooling" -Recurse -ErrorAction Ignore
	
	copy "lsystem/build/deployed-readme.txt" "$deploy_dir\README.txt"
	
	Remove-Item $out_file -Recurse -ErrorAction Ignore
	Compress-Archive -Path $deploy_dir\* -DestinationPath $out_file
	
	Remove-Item $deploy_dir -Recurse -ErrorAction Ignore
	
	"Deploy done: $out_file"
}

main

cd lsystem/build