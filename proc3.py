#!/bin/python
import sys
# sys.stdin=open('out_p2','r')

def softset(arr,index,val):
	if arr[index] in (None,val):
		arr[index]=val
	else:
		sys.stderr.write(f'Warning: softset index: {index:04x} existing: {arr[index]:04x} new: {val:04x}\n')


d=[int(x,16) for x in sys.stdin.read().strip().split('\n')]
print(hex(len(d)))
a=[None]*0x8000
i=0xfe31
sys.stderr.write(f"Starting address = {i:#06x}. Verify if it's correct.\n")
for x in d:
	softset(a,i//2,(0x3030-x)&0xffff)
	i=(i+0xd252)&0xffff

# print('\n'.join(f'= {i:04x}' if i!=None else 'None' for i in a))
# raise 1

l=0x01fe//2
# a[i]: sum of l words from address i*2

sys.stderr.write('DONE SETTING A\n')

r=[None]*0x8000
i=0

assert None not in a

def inv(x,y): # inefficient
	return next(a for a in range(y) if a*x%y==1)


# compute r[0]

if True:
	r[0]= 0xf8ce
	sys.stderr.write(f"First word in result = {r[0]:#06x}. Verify if it's correct.\n")
else:
	sumall=(sum(a)*inv(l,0x8000))&0xffff
	i=0
	s=0
	while (i&0x7fff)!=1:
		s+=a[i&0x7fff]
		i+=l
	# now s is the sum of i first elements
	r[0]=(s-sumall*(i>>15))&0xffff


i=0
for _ in range(0x8001):
	ipl=(i+l)&0x7fff
	si1,si=a[(i+1)&0x7fff],a[i]
	softset(r,ipl,(r[i]+si1-si)&0xffff)
	i=ipl


'''
r[0x3e6a//2:0x3e6a//2+5]=[0xf8ce,0xf46e,0xf87e,0xae1a,0xe1f0]

for _ in range(30):
	for i in [*range(0x8000),*reversed(range(0x8000))]:
		si1,si=a[(i+1)&0x7fff],a[i]
		if si!=None and si1!=None:
			ipl=(i+l)&0x7fff
			if r[i]!=None:
				softset(r,ipl,(r[i]+si1-si)&0xffff)
			if r[(i+l)&0x7fff]!=None:
				softset(r,i,(r[ipl]+si-si1)&0xffff)
'''

print('\n'.join(
	(f'= {i:04x}' if i!=None else 'None')
	+'\t'+
	(f'= {j:04x}' if j!=None else 'None')
	for i,j in zip(a,r)))

with open('out_bin.bin','wb') as f:
	f.write(bytes(y for x in r for y in (x&0xff,x>>8)))
