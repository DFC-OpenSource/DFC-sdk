"""Samples showing the parsing of common programming-language constructs

numbers
	integers
		int
		int_unsigned
		
	hexidecimal integers
		hex
		
	floats (including exponents, requring a '.' in the literal)
		float
			floats, with optional integer-only exponents
		float_floatexp
			floats, with optional integer or float exponents

	imaginary_number
		(float/int),[jJ]

	number
		hex/float/int
	number_full
		binary_number/imaginary_number/hex/float/int

	binary_number
		signed binary number
			1001001b or 1001001B bit-field format,
			optional sign
			can be used with number as (binary_number/number)

Interpreters:

	IntInterpreter
		int, int_unsigned
	HexInterpreter
		hex
	FloatInterpreter
		float
	FloatFloatExpInterpreter
		float_floatexp
	BinaryInterpreter
		binary_number
	ImaginaryInterpreter
		imaginary_number
	
"""
from simpleparse.parser import Parser
from simpleparse import common, objectgenerator
from simpleparse.common import chartypes
from simpleparse.dispatchprocessor import *
import string

c = {}

declaration = r"""
# sample for parsing integer and float numbers
# including hexidecimal numbers in 0xFFF format
sign             := [-+]+

<l_digits>          := digits
<l_hexdigits>       := hexdigits

decimal_fraction    := '.',int_unsigned?

# float which is explicitly a float, cannot be an integer
# because it includes a decimal point
explicit_base       := sign?, ((int_unsigned, decimal_fraction) / decimal_fraction / (int_unsigned,'.'))

exponent            := int
exponent_loose      := explicit_base/int

float               := explicit_base, ([eE],exponent)?
float_floatexp      := explicit_base, ([eE],exponent_loose)?
 
hex                 := sign?, '0', [xX], hexdigits
int_unsigned        := l_digits
int                 := sign?, l_digits
binary_digits       := [01]+
binary_number       := sign?, binary_digits,('b'/'B')

imaginary_number    := (float/int), [jJ]

##number            := binary_number/hex/float/int
number              := hex/float/int
number_full         := binary_number/imaginary_number/hex/float/int
"""

_p = Parser( declaration )
for name in ["int","hex", "int_unsigned", "number", "float", "binary_number", "float_floatexp", "imaginary_number", "number_full"]:
	c[ name ] = objectgenerator.LibraryElement(
		generator = _p._generator,
		production = name,
	)

if __name__ == "__main__":
	test()

common.share( c )

def _toInt( s, base ):
	try:
		return int( s, base)
	except TypeError:
		return string.atoi( s, base)
def _toLong( s, base ):
	try:
		return long( s, base)
	except TypeError:
		return string.atol( s, base)

class IntInterpreter(DispatchProcessor):
	"""Interpret an integer (or unsigned integer) string as an integer"""
	def __call__( self, (tag, left, right, children), buffer):
		try:
			return _toInt( buffer[left:right], 10)
		except ValueError:
			return _toLong( buffer[left:right], 10)
class HexInterpreter(DispatchProcessor):
	"""Interpret a hexidecimal integer string as an integer value"""
	def __call__( self, (tag, left, right, children), buffer):
		try:
			return _toInt( buffer[left:right], 16)
		except ValueError:
			return _toLong( buffer[left:right], 16)
		
class FloatFloatExpInterpreter(DispatchProcessor):
	"""Interpret a float string as an integer value
	Note: we're allowing float exponentiation, which
	gives you a nice way to write 2e.5
	"""
	def __call__( self, (tag, left, right, children), buffer):
		tag, l, r, _ = children[0]
		base = float( buffer[l:r] )
		if len(children) > 1:
			# figure out the exponent...
			exp = children[1]
			exp = buffer[ exp[1]:exp[2]]
##			import pdb
##			pdb.set_trace()
			exp = float( exp )
			
			base = base * (10** exp)
		return base
class FloatInterpreter(DispatchProcessor):
	"""Interpret a standard float value as a float"""
	def __call__( self, (tag, left, right, children), buffer):
		return float( buffer[left:right])

import sys
if hasattr( sys,'version_info') and sys.version_info[:2] > (2,0):
	class BinaryInterpreter(DispatchProcessor):
		def __call__( self, (tag, left, right, children), buffer):
			"""Interpret a bitfield set as an integer"""
			return _toInt( buffer[left:right-1], 2)
else:
	class BinaryInterpreter(DispatchProcessor):
		def __call__( self, (tag, left, right, children), buffer):
			"""Interpret a bitfield set as an integer, not sure this algo
			is correct, will see I suppose"""
			sign = 1
			if len(children) > 2:
				s = children[0]
				for schar in buffer[s[1]:s[2]]:
					if schar == '-':
						sign = sign * -1
				bits = buffer[children[1][1]:children[1][2]]
			else:
				bits = buffer[children[0][1]:children[0][2]]
			value = 0
			for bit in bits:
				value = (value << 1)
				if bit == '1':
					value = value + 1
			return value
		
class ImaginaryInterpreter( DispatchProcessor ):
	map = {
		"float":FloatInterpreter(),
		"int":IntInterpreter()
	}
	def __call__( self, (tag, left, right, children), buffer):
		"""Interpret a bitfield set as an integer, not sure this algo
		is correct, will see I suppose"""
		base = children[0]
		base = self.mapSet[base[0]](base, buffer)
		return base * 1j
	