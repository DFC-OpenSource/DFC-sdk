import unittest, string
from simpleparse.parser import Parser
from simpleparse.common import strings
from simpleparse import dispatchprocessor


parseTests = [
	# each production should match the whole of all of the first,
	# and not match any of the second...
	("string_triple_single", [
		"""'''this and that'''""",
		"""'''this \\''' '''""",
		"""''''''""",
		"""''''\\''''""",
	],[]),
	("string_triple_double", [
		'''"""this and that"""''',
		'''"""this \\""" """''',
		'''""""""''',
		'''""""\\""""''',
	],[]),
	("string_double_quote", [
		'"\\p"',
		'"\\""',
	],[]),
	("string",[
		"'this'",
		'"that"',
		r'"\b\f\n\r"',
		r'"\x32\xff\xcf"',
		r'"\032\033\055\077"',
		r'"\t\v\\\a\b\f\n\r"',
		r'"\t"',
		r'"\v"',
		r'"\""',
	], []),
]


class CommonTests(unittest.TestCase):
	def testBasic( self ):
		proc = dispatchprocessor.DispatchProcessor()
		setattr(proc, "string", strings.StringInterpreter())
		for production, yestable, notable in parseTests:
			p = Parser( "x := %s"%production, 'x')
			for data in yestable:
				if production == 'string':
					success, results, next = p.parse( data, processor=proc)
				else:
					success, results, next = p.parse( data)
				assert success and (next == len(data)), """Did not parse string %s as a %s result=%s"""%( repr(data), production, (success, results, next))
				assert results, """Didn't get any results for string %s as a %s result=%s"""%( repr(data), production, (success, results, next))
				if production == 'string':
					expected = eval( data, {},{})
					assert results[0] == expected, """Got different interpreted value for data %s, we got %s, expected %s"""%( repr(data), repr(results[0]), repr(expected))
			for data in notable:
				success, results, next = p.parse( data)
				assert not success, """Parsed %s of %s as a %s result=%s"""%( repr(data), production, (success, results, next))
				
		
		
def getSuite():
	return unittest.makeSuite(CommonTests, 'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
