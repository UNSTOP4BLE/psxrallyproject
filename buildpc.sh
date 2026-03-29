#am i a idiot? yes
rm -r build &&
cmake --preset pc-debug &&
cmake --build build/pc-debug &&
python makeassets.py -o build/pc-debug/assets &&
./build/pc-debug/main