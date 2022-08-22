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
assert 0 '{ return 0; }'
assert 42 '{ return 42; }'

# binary: +, -
assert 21 '{ return 5+20-4; }'

# ignore white space
assert 21 '{ return  5 + 20  - 4; }'
assert 21 '{ return  5 + 20  - 4 - 0 + 6 - 6 ; }'

# binary: +, -, *, /, ()
assert 47 '{ return 5+6*7; }'
assert 15 '{ return 5 * (9 - 6*(3/3)*1 + 0); }'
assert 2 '{ return (3+5)/2 - 2*(3-2); }'

# unary +, -
assert 0 '{ return -10 + 10; }'
assert 0 '{ return --+ + + -10 + - - 10 * -1 / -1; }'

# comparison operators: ==, !=, <, <=, >, >=
assert 0 '{ return 1 == 2; }'
assert 1 '{ return 1 == 1; }'
assert 0 '{ return -10 != -10; }'
assert 1 '{ return 10 != 20; }'
assert 0 '{ return (5+6*2)-12 > 5; }'
assert 1 '{ return (5+6*2)-12 >= 5; }'
assert 0 '{ return (5+6*2)-12 < 5*1/1; }'
assert 1 '{ return (5+6*2)-12 <= 5*1/1; }'

assert 1 '{ return 0<1; }'
assert 0 '{ return 1<1; }'
assert 0 '{ return 2<1; }'
assert 1 '{ return 0<=1; }'
assert 1 '{ return 1<=1; }'
assert 0 '{ return 2<=1; }'

assert 1 '{ return 1>0; }'
assert 0 '{ return 1>1; }'
assert 0 '{ return 1>2; }'
assert 1 '{ return 1>=0; }'
assert 1 '{ return 1>=1; }'
assert 0 '{ return 1>=2; }'

assert 3 '{ 1; 2; return 3; }'

# single letter variable, assign
assert 3 '{ a=3; return a; }'
assert 8 '{ a=3; z=5; return a+z; }'
assert 6 '{ a=b=3; return a+b; }'

# multi letter variable, assign
assert 3 '{ foo=3; return foo; }'
assert 8 '{ foo123=3; bar=5; return foo123+bar; }'
assert 6 '{ foo=2;f2=4; return f2+foo; }'

# return
assert 1 '{ return 1; 2; 3; }'
assert 2 '{ 1; return 2; 3; }'
assert 3 '{ 1; 2; return 3; }'

assert 3 '{ {1; {2;} {} return 3;} }'

# null statement
assert 5 '{ ;;; return 5; }'


echo ====TEST OK!=====
