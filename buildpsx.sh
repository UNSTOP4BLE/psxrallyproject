#am i a idiot? yes
rm -r build &&
cmake --preset psx-debug &&
cmake --build build/psx-debug &&
mkpsxiso -y rally.xml &&
#/Applications/PCSX-Redux.app/Contents/MacOS/PCSX-Redux  -pcdrv -pcdrvbase /Users/bruno/Desktop/psxrallyproject/build/psx-debug/assets -iso /Users/bruno/Desktop/psxrallyproject/PSXRP.cue
pcsx-redux -iso /Users/bruno/Desktop/psxrallyproject/PSXRP.cue
