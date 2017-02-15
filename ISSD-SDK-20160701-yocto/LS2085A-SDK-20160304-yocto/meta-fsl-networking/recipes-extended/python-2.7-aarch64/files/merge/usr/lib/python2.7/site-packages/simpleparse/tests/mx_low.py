"""Low-level matching tests for mx.TextTools"""
import unittest, pprint
from simpleparse.stt.TextTools import *

import string
from simpleparse.stt import TextTools
mxVersion = tuple(string.split( TextTools.__version__, '.')[:3])
from genericvalues import AnyInt, NullResult


class MXLowTests(unittest.TestCase):
	def doBasicTest(self, table, testvalue, expected, startPosition=0 ):
		result = tag( testvalue, table , startPosition)
		assert result == expected, '''\n\texpected:%s\n\tgot:%s\n'''%( expected, result )
	def testAllIn1( self ):
		"""Test simple AllIn command"""
		self.doBasicTest(
			(
				( "ab", AllIn, "ab", 0 ),
			),
			"abbaab",
			( 1,[("ab",0,6,None)],6),
		)
	def testAllIn2( self ):
		"""Test simple AllIn command ignore fail"""
		self.doBasicTest(
			(
				( "ab", AllIn, "ab", 1,1 ),
			),
			"c",
			( 1,[],0),
		)
	def testAllIn3( self ):
		"""Test simple AllIn command w 2 items"""
		self.doBasicTest(
			(
				( "ab", AllIn, "ab", 1,1 ),
				( "c", AllIn, "cde", 0 ),
			),
			"abbaabccdd",
			( 1,[
				("ab",0,6,None),
				("c",6,10,None),
				
			],10),
		)
	def testAllIn4( self ):
		"""Test simple AllIn command fail on second

		This should truncate the results list back to [], as well
		as returning 0 as length.  This is broken under
		mx.TextTools 2.1.0b1!
		"""
		self.doBasicTest(
			(
				( "ab", AllIn, "ab", 1,1 ),
				( "c", AllIn, "cde", 0 ),
			),
			"abbaab",
			( 0,[
			],AnyInt),
		)
	def testAllIn5( self ):
		"""Test simple AllIn command with None tagobj"""
		self.doBasicTest(
			(
				( None, AllIn, "ab", 0 ),
			),
			"abbaab",
			( 1,[],6),
		)
	def testAllNotIn1( self ):
		"""Test simple AllNotIn command"""
		self.doBasicTest(
			(
				( "ab", AllNotIn, "ab", 0 ),
			),
			"ccddee",
			( 1,[("ab",0,6,None)],6),
		)
	def testAllNotIn2( self ):
		"""Test simple AllNotIn command ignore fail"""
		self.doBasicTest(
			(
				( "ab", AllNotIn, "ab", 1,1 ),
			),
			"a",
			( 1,[],0),
		)
	def testAllNotIn3( self ):
		"""Test simple AllNotIn command w 2 items"""
		self.doBasicTest(
			(
				( "ab", AllNotIn, "ab", 1,1 ),
				( "c", AllNotIn, "cde", 0 ),
			),
			"ccddabbaab",
			( 1,[
				("ab",0,4,None),
				("c",4,10,None),
				
			],10),
		)
		

	def testIs1( self ):
		"""Test simple Is command"""
		self.doBasicTest(
			(
				( "ab", Is, "a", 0 ),
			),
			"abbaab",
			( 1,[("ab",0,1,None)],1),
		)
	def testIs2( self ):
		"""Test simple Is command ignore fail"""
		self.doBasicTest(
			(
				( "ab", Is, "a", 1,1),
			),
			"c",
			( 1,[],0),
		)
	
	def testIsIn1( self ):
		"""Test simple IsIn command"""
		self.doBasicTest(
			(
				( "ab", IsIn, "ab", 0 ),
			),
			"abbaab",
			( 1,[("ab",0,1,None)],1),
		)
	def testIsIn2( self ):
		"""Test simple IsIn command ignore fail"""
		self.doBasicTest(
			(
				( "ab", IsIn, "ab", 1,1),
			),
			"c",
			( 1,[],0),
		)

	def testIsNotIn1( self ):
		"""Test simple IsNotIn command"""
		self.doBasicTest(
			(
				( "ab", IsNotIn, "ab", 0 ),
			),
			"ccddee",
			( 1,[("ab",0,1,None)],1),
		)
	def testIsNotIn2( self ):
		"""Test simple IsNotIn command ignore fail"""
		self.doBasicTest(
			(
				( "ab", IsNotIn, "ab", 1,1),
			),
			"abb",
			( 1,[],0),
		)


	def testWord1( self ):
		"""Test simple Word command"""
		self.doBasicTest(
			(
				( "ab", Word, "ab", 0 ),
			),
			"ab",
			( 1,[("ab",0,2,None)],2),
		)
	def testWord2( self ):
		"""Test simple Word command ignore fail"""
		self.doBasicTest(
			(
				( "ab", Word, "ab", 1,1),
			),
			"cd",
			( 1,[],0),
		)
	def testWordStart1( self ):
		"""Test simple WordStart command"""
		self.doBasicTest(
			(
				( "ab", WordStart, "ab", 0 ),
			),
			"ddeeffab",
			( 1,[("ab",0,6,None)],6),
		)
	def testWordStart2( self ):
		"""Test simple WordStart command ignore fail"""
		self.doBasicTest(
			(
				( "ab", WordStart, "ab", 1,1),
			),
			"cdffgg",
			( 1,[],0),
		)
		
	def testWordEnd1( self ):
		"""Test simple WordEnd command"""
		self.doBasicTest(
			(
				( "ab", WordEnd, "ab", 0 ),
			),
			"ddeeffab",
			( 1,[("ab",0,8,None)],8),
		)
	def testWordEnd2( self ):
		"""Test simple WordEnd command ignore fail"""
		self.doBasicTest(
			(
				( "ab", WordEnd, "ab", 1,1),
			),
			"cdffgg",
			( 1,[],0),
		)

	def testAllInSet1( self ):
		"""Test simple AllInSet command"""
		self.doBasicTest(
			(
				( "ab", AllInSet, set("ab"), 0 ),
			),
			"abbaab",
			( 1,[("ab",0,6,None)],6),
		)
	def testAllInSet2( self ):
		"""Test simple AllInSet command ignore fail"""
		self.doBasicTest(
			(
				( "ab", AllInSet, set("ab"), 1,1 ),
			),
			"c",
			( 1,[],0),
		)

	def testIsInSet1( self ):
		"""Test simple IsInSet command"""
		self.doBasicTest(
			(
				( "ab", IsInSet, set("ab"), 0 ),
			),
			"abbaab",
			( 1,[("ab",0,1,None)],1),
		)
	def testIsInSet2( self ):
		"""Test simple IsInSet command ignore fail"""
		self.doBasicTest(
			(
				( "ab", IsInSet, set("ab"), 1,1),
			),
			"c",
			( 1,[],0),
		)
	if mxVersion >= ('2','1'):
		def testIsInCharSet1( self ):
			"""Test simple IsInCharSet command"""
			self.doBasicTest(
				(
					( "ab", IsInCharSet, CharSet("ab"), 0 ),
				),
				"abbaab",
				( 1,[("ab",0,1,None)],1),
			)
		def testIsInCharSet2( self ):
			"""Test simple IsInCharSet command ignore fail"""
			self.doBasicTest(
				(
					( "ab", IsInCharSet, CharSet("ab"), 1,1),
				),
				"c",
				( 1,[],0),
			)

		def testAllInCharSet1( self ):
			"""Test simple AllInSet command w/ CharSet object"""
			self.doBasicTest(
				(
					( "ab", AllInCharSet, CharSet("ab"), 0 ),
				),
				"abbaab",
				( 1,[("ab",0,6,None)],6),
			)
		def testAllInCharSet2( self ):
			"""Test simple AllInSet command ignore fail"""
			self.doBasicTest(
				(
					( "ab", AllInCharSet, CharSet("ab"), 1,1),
				),
				"ccd",
				( 1,[],0),
			)
	

		
def getSuite():
	return unittest.makeSuite(MXLowTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
