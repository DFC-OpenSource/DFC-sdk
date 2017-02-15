"""Low-level matching tests for mx.TextTools"""
import unittest, pprint
from simpleparse.stt.TextTools import *

import string
from simpleparse.stt import TextTools
mxVersion = tuple(string.split( TextTools.__version__, '.')[:3])

class MXHighTests(unittest.TestCase):
	def doBasicTest(self, table, testvalue, expected, startPosition=0 ):
		result = tag( testvalue, table , startPosition)
		assert result == expected, '''\n\texpected:%s\n\tgot:%s\n'''%( expected, result )

	### XXX Need to figure out what the heck loop is for and how to test it


	def testCall( self ):
		"""Test call-to-match Call command"""
		def function( text, start, end ):
			return end
	
		self.doBasicTest(
			(
				( "ab", Call, function, 0 ),
			),
			"cdffgg",
			( 1,[
				("ab",0,6,None),
			],6),
		)
	def testCall2( self ):
		"""Test call-to-match Call command with object instance"""
		class X:
			def __call__( self, text, start, end ):
				return end
	
		self.doBasicTest(
			(
				( "ab", Call, X(), 0 ),
			),
			"cdffgg",
			( 1,[
				("ab",0,6,None),
			],6),
		)
		
	def testCallArg( self ):
		"""Test call-to-match CallArg command"""
		def function( text, start, end, *arguments ):
			assert arguments == (1,2,3), """Passed arguments were not what we passed in"""
			return end
	
		self.doBasicTest(
			(
				( "ab", CallArg, (function,1,2,3), 0 ),
			),
			"cdffgg",
			( 1,[
				("ab",0,6,None),
			],6),
		)

	if mxVersion >= ('2','1'):
		def testsWordStart1( self ):
			"""Test simple sWordStart command"""
			for algo in [BOYERMOORE, TRIVIAL]:
				self.doBasicTest(
					(
						( "ab", sWordStart, TextSearch("ab", algorithm=algo), 0 ),
					),
					"ddeeffab",
					( 1,[("ab",0,6,None)],6),
				)
		def testsWordStart2( self ):
			"""Test simple sWordStart command ignore fail"""
			for algo in [BOYERMOORE, TRIVIAL]:
				self.doBasicTest(
					(
						( "ab", sWordStart, TextSearch("ab", algorithm=algo), 1,1),
					),
					"cdffgg",
					( 1,[],0),
				)
			
		def testsWordEnd1( self ):
			"""Test simple sWordEnd command"""
			for algo in [BOYERMOORE, TRIVIAL]:
				self.doBasicTest(
					(
						( "ab", sWordEnd, TextSearch("ab", algorithm=algo), 0 ),
					),
					"ddeeffab",
					( 1,[("ab",0,8,None)],8),
				)
		def testsWordEnd2( self ):
			"""Test simple sWordEnd command ignore fail"""
			for algo in [BOYERMOORE, TRIVIAL]:
				self.doBasicTest(
					(
						( "ab", sWordEnd, TextSearch("ab", algorithm=algo), 1,1),
					),
					"cdffgg",
					( 1,[],0),
				)


		def testsFindWord1( self ):
			"""Test simple sWordFind command"""
			for algo in [BOYERMOORE, TRIVIAL]:
				self.doBasicTest(
					(
						( "ab", sFindWord, TextSearch("ab", algorithm=algo), 0 ),
					),
					"ddeeffab",
					( 1,[("ab",6,8,None)],8),
				)
		def testsFindWord2( self ):
			"""Test simple sFindWord command ignore fail"""
			for algo in [BOYERMOORE, TRIVIAL]:
				self.doBasicTest(
					(
						( "ab", sFindWord, TextSearch("ab", algorithm=algo), 1,1),
					),
					"cdffgg",
					( 1,[],0),
				)
	else:
		def testsWordStart1( self ):
			"""Test simple sWordStart command"""
			self.doBasicTest(
				(
					( "ab", sWordStart, BMS("ab"), 0 ),
				),
				"ddeeffab",
				( 1,[("ab",0,6,None)],6),
			)
		def testsWordStart2( self ):
			"""Test simple sWordStart command ignore fail"""
			self.doBasicTest(
				(
					( "ab", sWordStart, BMS("ab"), 1,1),
				),
				"cdffgg",
				( 1,[],0),
			)
			
		def testsWordEnd1( self ):
			"""Test simple sWordEnd command"""
			self.doBasicTest(
				(
					( "ab", sWordEnd, BMS("ab"), 0 ),
				),
				"ddeeffab",
				( 1,[("ab",0,8,None)],8),
			)
		def testsWordEnd2( self ):
			"""Test simple sWordEnd command ignore fail"""
			self.doBasicTest(
				(
					( "ab", sWordEnd, BMS("ab"), 1,1),
				),
				"cdffgg",
				( 1,[],0),
			)


		def testsFindWord1( self ):
			"""Test simple sWordFind command"""
			self.doBasicTest(
				(
					( "ab", sFindWord, BMS("ab"), 0 ),
				),
				"ddeeffab",
				( 1,[("ab",6,8,None)],8),
			)
		def testsFindWord2( self ):
			"""Test simple sFindWord command ignore fail"""
			self.doBasicTest(
				(
					( "ab", sFindWord, BMS("ab"), 1,1),
				),
				"cdffgg",
				( 1,[],0),
			)
		

		
def getSuite():
	return unittest.makeSuite(MXHighTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
