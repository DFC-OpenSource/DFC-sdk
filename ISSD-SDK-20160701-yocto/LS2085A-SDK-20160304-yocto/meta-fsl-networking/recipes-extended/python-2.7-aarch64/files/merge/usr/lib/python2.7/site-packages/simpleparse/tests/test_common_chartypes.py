import unittest, string
from simpleparse.parser import Parser
from simpleparse.common import chartypes, timezone_names
from simpleparse import dispatchprocessor

fulltrans = string.maketrans("","")

class CommonTests(unittest.TestCase):
	def doBasicTest(self, definition, parserName, testValue, expected, ):
		result = Parser( definition).parse( testValue, parserName )
		assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )
	def _testSet( self, set, singleName, multiName ):
		"""Test multi-line definitions"""
		decl = """single := %s multiple := %s"""%( singleName, multiName )
		p = Parser(decl)
		notset = string.translate( fulltrans, fulltrans, set )
		for char in set:
			success, children, next = p.parse( char, singleName)
			assert success and (next == 1), """Parser for %s couldn't parse %s"""%( singleName, char )
		for char in notset:
			success, children, next = p.parse( char, singleName)
			assert (not success) and (next == 0), """Parser for %s parsed %s"""%( singleName, char )
			success, children, next = p.parse( char, multiName)
			assert (not success) and (next == 0), """Parser for %s parsed %s"""%( multiName, char )
		success, children, next = p.parse( set, multiName)
		assert success and (next == len(set)), """Parser for %s couldn't parse full set of chars, failed at %s"""%( multiName, set[next:] )
	def testBasic( self ):
		for set, single, multiple in (
			("digits", "digit", "digits"),
			("uppercase", "uppercasechar", "uppercase"),
			("lowercase", "lowercasechar", "lowercase"),
			("letters", "letter", "letters"),
			("whitespace", "whitespacechar", "whitespace"),
			("octdigits", "octdigit", "octdigits"),
			("hexdigits", "hexdigit", "hexdigits"),
			("printable", "printablechar", "printable"),
			("punctuation", "punctuationchar", "punctuation"),

			("ascii_lowercase", "ascii_lowercasechar", "ascii_lowercase"),
			("ascii_uppercase", "ascii_uppercasechar", "ascii_uppercase"),
		):
			try:
				set = getattr( string, set)
				self._testSet(
					set,
					single,
					multiple,
				)
			except AttributeError:
				pass
	def testEOF( self ):
		p = Parser( """this := 'a',EOF""", 'this')
		success, children, next = p.parse( 'a' )
		assert success, """EOF didn't match at end of string"""
	def testEOFFail( self ):
		p = Parser( """this := 'a',EOF""", 'this')
		success, children, next = p.parse( 'a ' )
		assert not success, """EOF matched before end of string"""
	
	def testTZ( self ):
		names = timezone_names.timezone_mapping.keys()
		names.sort() # tests that the items don't match shorter versions...
		decl = Parser("""this := (timezone_name, ' '?)+""", 'this')
		proc = dispatchprocessor.DispatchProcessor()
		proc.timezone_name = timezone_names.TimeZoneNameInterpreter()
		text = string.join( names, ' ')
		success, result, next = decl.parse( text, processor = proc )
		assert success, """Unable to complete parsing the timezone names, stopped parsing at char %s %s"""%(next, text[next:])
		assert result == map( timezone_names.timezone_mapping.get, names), """Got different results for interpretation than expected (expected first, recieved second)\n%s\n%s"""%(map( timezone_names.timezone_mapping.get, names), result)
		
		
			
		
def getSuite():
	return unittest.makeSuite(CommonTests, 'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
