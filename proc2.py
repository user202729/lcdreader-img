#!/bin/python
'''
Shrink (gap) (+-1, may be fractional) items to one. Some error on boundary are allowed.
'''

import sys
import pyqtgraph
import numpy

DEBUG=0

with sys.stdin as f:
# with open(sys.argv[1],'r') as f:
	d=f.read().strip().split('\n')

def check_gap():
	x=[sum(x!=y for x,y in zip(a,b)) for a,b in zip(d,d[1:])]
	x=numpy.array(x)
	x=x[:1000]
	print(len(x))

	y=numpy.correlate(x,x,mode='full')
	y=y[y.size//2:]
	y=y[:30]
	print(len(y))

	pyqtgraph.plot(y)

if 0:
	check_gap()
	input()
	sys.exit()

gap=9.5

def r(i):
	i=int(i)

	j=i
	while j>0 and d[j-1]==d[i]:
		j-=1

	k=i+1
	while k<len(d) and d[k]==d[i]:
		k+=1
	return (j,k)

i=1
while True:
	j,k=r(i)
	if 2<=k-j<=gap:
		i=(j+k)/2
		break
	else:
		i=k

mark=[False]*len(d)

maxfix=0.5
cnt=0

if DEBUG:
	import collections
	countlen=collections.Counter()

while i<len(d):
	cnt+=1
	if DEBUG:
		# countlen[k-j]+=1
		if k-j<=2:
			print('NOTE')
		print(f'{cnt}\t{int(i)}\t{d[int(i)]}\t{k-j}\t{abs(i-(k+j)/2):.2f}')
	else:
		print(d[int(i)])
	mark[int(i)]=True

	i+=gap
	if i>=len(d):
		break
	j,k=r(i)
	m=(j+k)/2
	oldi=i
	i+=min(maxfix,max(-maxfix,m-i)) # adjust i closer to m
	# print(f'\t\t\t\t\t{oldi:.2f}\t{m}\t{i:.2f}')

# for m,di in zip(mark,d):
# 	print('*' if m else ' ',di)

if DEBUG:
	print(countlen)
