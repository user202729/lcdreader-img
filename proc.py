#!/bin/python
'''
Visualize large continuous blocks.

NOT USEFUL IN PRACTICE.
'''

import sys
d=sys.stdin.read().strip().split('\n')

N=len(d)
i=1
ans=[]
while i<N:
	if d[i]==d[i-1]:
		j=i+1
		while j<N and d[j]==d[i]:j+=1

		ans.append((i-1,j,d[i]))

		i=j
	else:
		i+=1

lasta=None
print("vim: ts=8\nDiff\tStart\tLen\tData")
for a,b,c in ans:
	print(f"{'' if lasta==None else a-lasta}\t{a}\t{b-a}\t{c}")
	lasta=a
