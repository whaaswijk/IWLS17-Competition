#!/usr/bin/env python
from sympy import *

alphabet = ['a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z']

# Compute the number of symbols for the specified y-cardinality
def nsymbols(cardinality):
	return int(((cardinality + 2)*(cardinality + 1)) / 2)

def get_nsymbols(n):
	return symbols(' '.join(alphabet[0:n]))

def symbols_for_yn(n):
	return list(get_nsymbols(nsymbols(n)))

def drop_bottom(n, symbols):
	res = []
	symbols_on_level = 1
	symbols_appended = 0
	for i in range(n):
		add = symbols[symbols_appended:symbols_appended+symbols_on_level]
		symbols_appended += symbols_on_level
		res = res + add
		symbols_on_level += 1
	return res

def drop_right(n, symbols):
	res = []
	add_on_level = 0
	skip = 0
	symbols_on_level = 1
	for i in range(n+1):
		add = symbols[skip:skip+add_on_level]
		res = res + add
		skip += symbols_on_level
		symbols_on_level += 1
		add_on_level += 1
	return res

def drop_left(n, symbols):
	res = []
	add_on_level = 0
	skip = 0
	symbols_on_level = 1
	symbols_encountered = 0
	for i in range(n+1):
		add = symbols[skip:skip+add_on_level]
		res = res + add
		symbols_encountered += symbols_on_level
		skip = symbols_encountered + 1
		symbols_on_level += 1
		add_on_level += 1
	return res

def rec_formula_for_yn(n, symbols):
	if n == 0:
		return symbols[0]
	bottom_formula = rec_formula_for_yn(n-1, drop_bottom(n, symbols))
	right_formula = rec_formula_for_yn(n-1, drop_right(n, symbols))
	left_formula = rec_formula_for_yn(n-1, drop_left(n, symbols))
	return (bottom_formula & right_formula) | (bottom_formula & left_formula) | (left_formula & right_formula)

def formula_for_yn(n):
	symbols = symbols_for_yn(n)
	bottom_formula = rec_formula_for_yn(n-1, drop_bottom(n, symbols))
	right_formula = rec_formula_for_yn(n-1, drop_right(n, symbols))
	left_formula = rec_formula_for_yn(n-1, drop_left(n, symbols))
	f = (bottom_formula & right_formula) | (bottom_formula & left_formula) | (left_formula & right_formula)
	return to_dnf(f, simplify=True)

n = 3
y_formula = formula_for_yn(n)
init_printing()
#print('Y{}_formula: {}'.format(n, y_formula))
print(y_formula)

