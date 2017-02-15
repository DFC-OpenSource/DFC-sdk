"""Test the print-to-python-file module

This just uses the simpleparsegrammar declaration, which is
parsed, then linearised, then loaded as a Python module.
"""
import os, unittest
import test_grammarparser
testModuleFile = 'test_printers_garbage.py'


class PrintersTests(test_grammarparser.SimpleParseGrammarTests):
	def setUp( self ):
		from simpleparse import simpleparsegrammar, parser, printers, baseparser
		p = parser.Parser( simpleparsegrammar.declaration, 'declarationset')
		open(testModuleFile,'w').write(printers.asGenerator( p._generator ))
		import test_printers_garbage
		reload( test_printers_garbage )
		
		class RParser( test_printers_garbage.Parser, baseparser.BaseParser ):
			pass

		self.recursiveParser = RParser()
	def tearDown( self ):
		try:
			os.remove( testModuleFile )
		except IOError, err:
			pass
	def doBasicTest(self, parserName, testValue, expected, ):
		result = self.recursiveParser.parse( testValue, production=parserName )
		assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )

def getSuite():
	return unittest.makeSuite(PrintersTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
		
