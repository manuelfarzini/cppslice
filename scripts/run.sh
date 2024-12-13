rm -rf .cache
rm compile_commands.json
make clean
bear -- make
clear
./cppslice.x
