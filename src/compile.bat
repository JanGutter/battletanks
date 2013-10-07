msbuild.exe bt.vcxproj /p:Configuration=Release
copy Release\bt.exe .
del /q Release\*.*
