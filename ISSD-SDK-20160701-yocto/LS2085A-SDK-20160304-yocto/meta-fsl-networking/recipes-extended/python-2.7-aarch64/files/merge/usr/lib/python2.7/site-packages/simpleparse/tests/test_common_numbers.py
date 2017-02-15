import unittest, string
from simpleparse.parser import Parser
from simpleparse.common import numbers
from simpleparse import dispatchprocessor

_data = [
	(
		"int_unsigned", numbers.IntInterpreter,
		[ # should match, value, length that should match, expected result
			("0 ", 1, 0),
			("1 ", 1, 1),
			("23 ",2, 23),
			("0x ", 1,0),
			("0. ", 1,0),
		],
		[ # should not match...
			".0",
			"a",
		],
	),
	(
		"int", numbers.IntInterpreter,
		[ # should match, value, length that should match, expected result
			("0 ", 1, 0),
			("1 ", 1, 1),
			("23 ",2, 23),
			("0x ", 1,0),
			("0. ", 1,0),
			("+0 ", 2, 0),
			("+1 ", 2, 1),
			("+23 ",3, 23),
			("+0x ", 2,0),
			("+0. ", 2,0),
			("-0 ", 2, 0),
			("-1 ", 2, -1),
			("-23 ",3, -23),
			("-0x ", 2,0),
			("-0. ", 2,0),
		],
		[ # should not match...
			".0",
			"a",
			"+.0",
			"+a",
			"-.0",
			"-a",
		],
	),
	(
		"hex", numbers.HexInterpreter,
		[ # should match, value, length that should match, expected result
			("0x0 ", 3, 0),
			("0x1 ", 3, 1),
			("0x23 ",4, 35),
			("0x0x ", 3,0),
			("0x0. ", 3,0),
			("+0x0 ", 4, 0),
			("+0x1 ", 4, 1),
			("+0x23 ",5, 35),
			("+0x0x ", 4,0),
			("+0x0. ", 4,0),
			("-0x0 ", 4, 0),
			("-0x1 ", 4, -1),
			("-0x23 ",5, -35),
			("-0x0x ", 4,0),
			("-0x0. ", 4,0),
			("0xa ", 3, 10),
			("0xaaaaaaaaaaaaaaaaa ", 19, 196765270119568550570L),
			("0xA ", 3, 10),
			("0xAAAAAAAAAAAAAAAAA ", 19, 196765270119568550570L),
		],
		[ # should not match...
			".0",
			"a",
			"+.0",
			"+a",
			"-.0",
			"-a",
			"0x ",
			"0xg",
			"0x",
		],
	),
	(
		"binary_number", numbers.BinaryInterpreter,
		[ # should match, value, length that should match, expected result
			("0b0 ", 2, 0),
			("1b0 ", 2, 1),
			("10b0 ", 3, 2),
			("10000000000b0 ", 12, 1024),
			("0B0 ", 2, 0),
			("1B0 ", 2, 1),
			("10B0 ", 3, 2),
			("10000000000B0 ", 12, 1024),
		],
		[ # should not match...
			".0",
			"a",
			"+.0",
			"+a",
			"-.0",
			"-a",
			"0x ",
			"0xg",
			"0x",
		],
	),
	(
		"float", numbers.FloatInterpreter,
		[ # should match, value, length that should match, expected result
			("0. ", 2, 0),
			("1. ", 2, 1),
			("23. ",3, 23),
			(".0 ", 2, 0),
			(".1 ", 2, .1),
			(".23 ",3, .23),
			("0.0x ", 3,0),
			("1.1x ", 3,1.1),
			("2000000.22222222x ", 16, 2000000.22222222),
			("1.1e20 ", 6, 1.1e20),
			("1.1e-20 ",7, 1.1e-20),
			("-1.1e20 ", 7, -1.1e20),
		],
		[ # should not match...
			"0x.0",
			"23",
			"-23",
			"-43*2a",
			"+23",
			"-a",
		],
	),
	(
		"float_floatexp", numbers.FloatFloatExpInterpreter,
		[ # should match, value, length that should match, expected result
			("0. ", 2, 0),
			("1. ", 2, 1),
			("23. ",3, 23),
			(".0 ", 2, 0),
			(".1 ", 2, .1),
			(".23 ",3, .23),
			("0.0x ", 3,0),
			("1.1x ", 3,1.1),
			("2000000.22222222x ", 16, 2000000.22222222),
			("1.1e20 ", 6, 1.1* (1e20)),
			("1.1e-20 ",7, 1.1* (1e-20)),
			("-1.1e20 ", 7, -1.1* (1e20)),
			("1.1e20.34 ", 9, 1.1* (10 ** 20.34)),
			("1.1e-.34 ", 8, 1.1*( 10 ** -.34)),
		],
		[ # should not match...
			"0x.0",
			"23",
			"-23",
			"-43*2a",
			"+23",
			"-a",
		],
	),
]		
	 

class CommonTests(unittest.TestCase):
	def testBasic( self ):
		for production, processor, yestable, notable in _data:
			p = Parser( "x := %s"%production, 'x')
			proc = dispatchprocessor.DispatchProcessor()
			setattr(proc, production, processor())
			for data, length, value in yestable:
				success, results, next = p.parse( data, processor = proc)
				assert next == length, """Did not parse string %s of %s as a %s result=%s"""%( repr(data[:length]), repr(data), production, (success, results, next))
				assert results[0] == value, """Didn't get expected value from processing value %s, expected %s, got %s"""%( data[:length], value, results[0])
				
			for data in notable:
				success, results, next = p.parse( data)
				assert not success, """Parsed %s of %s as a %s result=%s"""%( repr(data[:length]), repr(data), production, (success, results, next))
				
		
		
def getSuite():
	return unittest.makeSuite(CommonTests, 'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
