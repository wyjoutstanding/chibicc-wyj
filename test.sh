#!/bin/bash

assert() {
    expected="$1"
    input="$2"

    ./chibicc-wyj "$input" > tmp.s || exit
    gcc -static -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 42 42

assert 21 '5+20-4'

assert 21 "  5 + 20  - 4"
assert 21 "  5 + 20  - 4 - 0 + 6 - 6 "

assert 47 "5+6*7"
assert 15 "5 * (9 - 6*(3/3)*1 + 0)"
assert 2 "(3+5)/2 - 2*(3-2)"

echo ====TEST OK!=====
