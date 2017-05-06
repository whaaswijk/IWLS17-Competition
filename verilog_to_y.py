#!/usr/bin/env python
import sys, collections, re
from enum import Enum
import copy

class ylit:
	def __init__(self, x, is_c=False):
		self.x = x
		self.is_c = is_c

	def complement(self):
		cpy = copy.copy(self)
		cpy.is_c = not cpy.is_c
		return cpy

class ygate(object):
	def __init__(self, is_po=False, idx = -1):
		self.is_po = is_po
		self.idx = idx

	def to_str(self, yig):
		return ''

	def set_po(self, is_po, idx):
		self.is_po = is_po
		self.idx = idx

class zerogate(ygate):
	def __init__(self):
		super(zerogate, self).__init__()
		pass

	def to_str(self, yig):
		if self.is_po:
			return 'o{} = Y0(0)'.format(self.idx)
		else:
			return 'w{} = Y0(0)'.format(self.idx)

class onegate(ygate):
	def __init__(self):
		super(ygate, self).__init__()
		pass
	
	def to_str(self, yig):
		if self.is_po:
			return 'o{} = Y0(1)'.format(self.idx)
		else:
			return 'w{} = Y0(1)'.format(self.idx)

class pigate(ygate):
	def __init__(self, lit):
		super(pigate, self).__init__()
		self.lit = lit
	
	def to_str(self, yig):
		raise Exception('Invalid gate type')

class inv1gate(ygate):
	def __init__(self, lit):
		super(inv1gate, self).__init__()
		self.lit = lit
	
	def to_str(self, yig):
		i = self.lit
		if self.is_po:
			return 'o{} = Y0(~{})'.format(self.idx, yig.var_to_str(i.x))
		return 'w{} = Y0(~{})'.format(self.idx, yig.var_to_str(i.x))

class and2gate(ygate):
	def __init__(self, lit1, lit2):
		super(and2gate, self).__init__()
		self.lit1 = lit1
		self.lit2 = lit2

	def to_str(self, yig):
		pfx = 'o' if self.is_po else 'w'
		return '{}{} = Y1({}{}, {}{}, 0)'.format(pfx, self.idx, 
				'~' if self.lit1.is_c else '', yig.var_to_str(self.lit1.x),
				'~' if self.lit2.is_c else '', yig.var_to_str(self.lit2.x))

class or2gate(ygate):
	def __init__(self, lit1, lit2):
		super(or2gate, self).__init__()
		self.lit1 = lit1
		self.lit2 = lit2

	def to_str(self, yig):
		pfx = 'o' if self.is_po else 'w'
		return '{}{} = Y1({}{}, {}{}, 1)'.format(pfx, self.idx, 
				'~' if self.lit1.is_c else '', yig.var_to_str(self.lit1.x),
				'~' if self.lit2.is_c else '', yig.var_to_str(self.lit2.x))

class yngate(ygate):
	def __init__(self, n, lits):
		super(yngate, self).__init__()
		self.n = n
		self.lits = lits
	
	def to_str(self, yig):
		pfx = 'o' if self.is_po else 'w'
		buf = '{}{} = Y{}('.format(pfx, self.idx, self.n)
		litstrings = ['{}{}'.format('~' if lit.is_c else '', yig.var_to_str(lit.x)) for lit in self.lits]
		buf += ', '.join(litstrings)
		buf += ')'
		return buf

class yig:
	def __init__(self, ninputs, noutputs):
		self.ninputs = ninputs
		self.noutputs = noutputs
		self.gates = []
		self.nodemap = {}

	def create_input(self):
		l = ylit(len(self.gates))
		self.gates.append(pigate(l.x))
		return l

	def create_gate(self, yg):
		l = ylit(len(self.gates))
		self.gates.append(yg)
		return l

	def var_to_str(self, x):
		if x < self.ninputs: # It's a PI
			return 'i{}'.format(x+1)
		else:
			gate = self.gates[x]
			if gate.is_po:
				return 'o{}'.format(gate.idx)
			else:
				return 'w{}'.format(gate.idx)

	def finalize(self, outnames, nodemap):
		wire_idx = 1
		po_idx = 1
		for name in outnames:
			lit = nodemap[name]
			self.gates[lit.x].set_po(True, po_idx)
			po_idx += 1
		for gate in self.gates[self.ninputs:]:
			if not gate.is_po:
				gate.idx = wire_idx
				wire_idx += 1

	def write(self, filename):
		with open(filename, 'w') as f:
			f.write('.i {}\n'.format(self.ninputs))
			f.write('.o {}\n'.format(self.noutputs))
			f.write('.w {}\n'.format(len(self.gates)-self.ninputs))
			for gate in self.gates[self.ninputs:]:
				gate_str = gate.to_str(self)
				f.write('{}\n'.format(gate_str))
			f.write('.e\n')

class GateType(Enum):
	ZERO = 0
	ONE = 1
	INV1 = 2
	AND2 = 3
	OR2 = 4
	Y1 = 5

class VGate:
	def __init__(self, gate_type, vinputs):
		self.gate_type = gate_type
		self.vinputs = vinputs

def flatten(l):
	for el in l:
		if isinstance(el, collections.Iterable) and not isinstance(el, (str, bytes)):
			yield from flatten(el)
		else:
			yield el

def def_line_to_list(line):
	raw_cmpts = line.split()
	raw_cmpts = list(flatten([s.split(',') for s in raw_cmpts]))
	raw_cmpts = [s[:-1] if s.endswith(',') or s.endswith(';') else s for s in raw_cmpts[1:]]
	return [s for s in raw_cmpts if len(s) > 0]

def parse_file(fhandle):
	name = None
	innames = []
	outnames = []
	wirenames = []
	wiremap = {}
	lines = fhandle.readlines()
	parsing_module = False
	parsing_inputs = False
	parsing_outputs = False
	parsing_wires = False
	buf = None
	for i, line in enumerate(lines):
		tline = line.strip()
		if len(tline) == 0: # empty line
			continue
		elif tline.startswith('//'): # comment
			continue
		elif tline.startswith('module'): # module definition
			cmpts = tline.split()
			name = cmpts[1]
			parsing_module = True
		elif tline.startswith('input'): # PI definitions
			parsing_module = False
			buf = tline
			if tline.endswith(';'): # last line
				innames = def_line_to_list(buf)
			else:
				parsing_inputs = True
		elif parsing_inputs:
			buf += tline
			if tline.endswith(';'): # last line
				innames = def_line_to_list(buf)
				parsing_inputs = False
		elif tline.startswith('output'): # PO definitions
			buf = tline
			if tline.endswith(';'): # last line
				outnames = def_line_to_list(buf)
			else:
				parsing_outputs = True
		elif parsing_outputs:
			buf += tline
			if tline.endswith(';'): # last line
				outnames = def_line_to_list(buf)
				parsing_outputs = False
		elif tline.startswith('wire'): # Wire definitions
			buf = tline
			if tline.endswith(';'): # last line
				wirenames = def_line_to_list(buf)
			else:
				parsing_wires = True
		elif parsing_wires:
			buf += tline
			if tline.endswith(';'): # last line
				wirenames = def_line_to_list(buf)
				parsing_wires = False
		elif tline.startswith('ZERO'):
			m = re.search(r'O\([a-zA-Z0-9]+\)', tline)
			wirename = m.group(0)[2:-1]
			is_po = wirename.startswith('po')
			wiremap[wirename] = VGate(GateType.ZERO, [])
		elif tline.startswith('ONE'):
			m = re.search(r'O\([a-zA-Z0-9]+\)', tline)
			wirename = m.group(0)[2:-1]
			is_po = wirename.startswith('po')
			wiremap[wirename] = VGate(GateType.ONE, [])
		elif tline.startswith('INV1'):
			m = re.search(r'O\([a-zA-Z0-9]+\)', tline)
			wirename = m.group(0)[2:-1]
			is_po = wirename.startswith('po')
			m = re.search(r'a\([a-zA-Z0-9]+\)', tline)
			in1name = m.group(0)[2:-1]
			wiremap[wirename] = VGate(GateType.INV1, [in1name])
		elif tline.startswith('AND2'):
			m = re.search(r'O\([a-zA-Z0-9]+\)', tline)
			wirename = m.group(0)[2:-1]
			is_po = wirename.startswith('po')
			m = re.search(r'a\([a-zA-Z0-9]+\)', tline)
			in1name = m.group(0)[2:-1]
			in1pi = in1name.startswith('pi')
			in1po = in1name.startswith('po')
			m = re.search(r'b\([a-zA-Z0-9]+\)', tline)
			in2name = m.group(0)[2:-1]
			in2pi = in2name.startswith('pi')
			in2po = in2name.startswith('po')
			wiremap[wirename] = VGate(GateType.AND2, [in1name, in2name])
		elif tline.startswith('OR2'):
			m = re.search(r'O\([a-zA-Z0-9]+\)', tline)
			wirename = m.group(0)[2:-1]
			is_po = wirename.startswith('po')
			m = re.search(r'a\([a-zA-Z0-9]+\)', tline)
			in1name = m.group(0)[2:-1]
			in1pi = in1name.startswith('pi')
			in1po = in1name.startswith('po')
			m = re.search(r'b\([a-zA-Z0-9]+\)', tline)
			in2name = m.group(0)[2:-1]
			in2pi = in2name.startswith('pi')
			in2po = in2name.startswith('po')
			wiremap[wirename] = VGate(GateType.OR2, [in1name, in2name])
		elif tline.startswith('Y1'):
			m = re.search(r'O\([a-zA-Z0-9]+\)', tline)
			wirename = m.group(0)[2:-1]
			m = re.search(r'a\([a-zA-Z0-9]+\)', tline)
			in1name = m.group(0)[2:-1]
			in1pi = in1name.startswith('pi')
			in1po = in1name.startswith('po')
			m = re.search(r'b\([a-zA-Z0-9]+\)', tline)
			in2name = m.group(0)[2:-1]
			in2pi = in2name.startswith('pi')
			in2po = in2name.startswith('po')
			m = re.search(r'b\([a-zA-Z0-9]+\)', tline)
			in3name = m.group(0)[2:-1]
			in3pi = in3name.startswith('pi')
			in3po = in3name.startswith('po')
			is_po = wirename.startswith('po')
			wiremap[wirename] = VGate(GateType.Y1, [in1name, in2name,in3name])
		elif tline.startswith('endmodule'):
			break
		elif parsing_module:
			continue
		else:
			raise Exception('Parsing error at line {}: {}'.format(i, tline))

	y = yig(len(innames), len(outnames))
	nodemap = {}
	for inname in innames:
		nodemap[inname] = y.create_input()

	converged = False
	while not converged:
		converged =  True
		for wirename,vgate in wiremap.items():
			if wirename in nodemap:
				continue
			g = None
			inputs_defined = True
			for vin in vgate.vinputs:
				if vin not in nodemap:
					converged = False
					inputs_defined = False
			if not inputs_defined:
				continue

			if vgate.gate_type == GateType.ZERO:
				g = zerogate()
			elif vgate.gate_type == GateType.ONE:
				g = onegate()
			elif vgate.gate_type == GateType.INV1:
				lit1 = nodemap[vgate.vinputs[0]]
				g = inv1gate(lit1)
			elif vgate.gate_type == GateType.AND2:
				lit1 = nodemap[vgate.vinputs[0]]
				lit2 = nodemap[vgate.vinputs[1]]
				g = and2gate(lit1, lit2)
			elif vgate.gate_type == GateType.OR2:
				lit1 = nodemap[vgate.vinputs[0]]
				lit2 = nodemap[vgate.vinputs[1]]
				g = or2gate(lit1, lit2)
			elif vgate.gate_type == GateType.Y1:
				lit1 = nodemap[vgate.vinputs[0]]
				lit2 = nodemap[vgate.vinputs[1]]
				lit3 = nodemap[vgate.vinputs[2]]
				g = yngate(1, [lit1, lit2, lit3])
			else:
				raise Exception('Unknown gate type: {}'.format(vgate.gate_type))
			nodemap[wirename] = y.create_gate(g)

	y.finalize(outnames, nodemap)
	
	return y



if len(sys.argv) != 2:
	print('Usage: {} <filename>'.format(sys.argv[0]))
	sys.exit(1)

filename = str(sys.argv[1]);
y = None
with open(filename, 'r') as fhandle:
	y = parse_file(fhandle)

y.write(filename + '.yig')
