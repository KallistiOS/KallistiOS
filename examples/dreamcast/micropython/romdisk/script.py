# KallistiOS ##version##
#
# script.py
# Copyright (C) 2023 Aaron Glazer
#

def f(n):
    return n * n
def g(n):
    return n ** n

sum = 0
for i in range(10):
    print('iter {:08}'.format(i))
    sum += i

try:
    1//0
except Exception as er:
    print('caught exception', repr(er))

import gc
print('run GC collect')
gc.collect()

print('finish')
