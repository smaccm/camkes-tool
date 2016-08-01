#!/usr/bin/env python
import sys
import re

def main(argv):
	adl_regex = re.compile(r'ADL\s*:=\s*([\w.]*)')
	vm_regex = re.compile(r'VM_CONFIG\s*:=\s*([\w.]*)')
	# Get the ADL from the Makefile
	# Also get the VM config if it is a VMs
	if len(argv) >= 1:
		with open(argv[0]) as makefile:
			lines = makefile.readlines()
		for line in lines:
			search1 = adl_regex.search(line)
			search2 = vm_regex.search(line)
			if search1:
				adl = search1.group(1)
			if search2:
				vm = search2.group(1)
	if vm:
		print adl, vm
	else:
		print adl

if __name__ == "__main__":
    main(sys.argv[1:])