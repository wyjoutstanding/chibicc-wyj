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

# integer
assert 0 0
assert 42 42

# binary: +, -
assert 21 '5+20-4'

# ignore white space
assert 21 "  5 + 20  - 4"
assert 21 "  5 + 20  - 4 - 0 + 6 - 6 "

# binary: +, -, *, /, ()
assert 47 "5+6*7"
assert 15 "5 * (9 - 6*(3/3)*1 + 0)"
assert 2 "(3+5)/2 - 2*(3-2)"

# unary +, -
assert 0 "-10 + 10"
assert 0 "--+ + + -10 + - - 10 * -1 / -1"

# comparison operators: ==, !=, <, <=, >, >=
assert 0 "1 == 2"
assert 1 "1 == 1"
assert 0 "-10 != -10"
assert 1 "10 != 20"
assert 0  "(5+6*2)-12 > 5"
assert 1  "(5+6*2)-12 >= 5"
assert 0  "(5+6*2)-12 < 5*1/1"
assert 1  "(5+6*2)-12 <= 5*1/1"

assert 1 '0<1'
assert 0 '1<1'
assert 0 '2<1'
assert 1 '0<=1'
assert 1 '1<=1'
assert 0 '2<=1'

assert 1 '1>0'
assert 0 '1>1'
assert 0 '1>2'
assert 1 '1>=0'
assert 1 '1>=1'
assert 0 '1>=2'

echo ====TEST OK!=====
