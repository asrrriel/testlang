set -e

make -C test
./test/bin/test test/lexer/lexer.c