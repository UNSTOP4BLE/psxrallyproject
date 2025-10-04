#am i a idiot? yes
rm -r build &&
cmake --preset psx-debug &&
cmake --build build/psx-debug &&
/Applications/PCSX-Redux.app/Contents/MacOS/PCSX-Redux  -pcdrv -pcdrvbase /Users/bruno/Desktop/psxrallyproject/build/psx-debug/assets  -exe /Users/bruno/Desktop/psxrallyproject/build/psx-debug/PSXRallyProject.psexe
