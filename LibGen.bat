rmdir /s /q Library
mkdir Library\include
mkdir Library\bin\x86
mkdir Library\lib\x86
mkdir Library\bin\x64
mkdir Library\lib\x64
copy WinFW\*.hpp Library\include
copy Release\*.dll Library\bin\x86
copy Release\*.lib Library\lib\x86
copy x64\Release\*.dll Library\bin\x64
copy x64\Release\*.lib Library\lib\x64