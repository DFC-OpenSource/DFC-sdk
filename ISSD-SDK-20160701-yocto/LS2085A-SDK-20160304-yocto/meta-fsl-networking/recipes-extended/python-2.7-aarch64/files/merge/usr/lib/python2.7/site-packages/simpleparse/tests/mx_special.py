"""Low-level matching tests for mx.TextTools"""
import unittest, pprint
from simpleparse.stt.TextTools import *

import string
from simpleparse.stt import TextTools
mxVersion = tuple(string.split( TextTools.__version__, '.')[:3])
from genericvalues import AnyInt, NullResult

class MXSpecialTests(unittest.TestCase):
	def doBasicTest(self, table, testvalue, expected, startPosition=0 ):
		result = tag( testvalue, table , startPosition)
		assert result == expected, '''\n\texpected:%s\n\tgot:%s\n'''%( expected, result )
	def testFail1( self ):
		"""Test Fail command"""
		self.doBasicTest(
			(
				( "ab", Fail, None, 0 ),
			),
			"abbaab",
			( 0,[
			],AnyInt),
		)
	def testFail2( self ):
		"""Test Fail command with ignore fail (Jump)"""
		self.doBasicTest(
			(
				( "ab", Fail, None, 1),
			),
			"abbaab",
			( 1,[
			],0),
		)

	def testSkip1( self ):
		"""Test Skip command"""
		self.doBasicTest(
			(
				( "ab", Skip, 1, 0 ),
			),
			"abbaab",
			( 1,[
				("ab",0,1,None),
			],1),
		)
	def testSkip2( self ):
		"""Test Skip command with negative to before buffer

		Note: I don't like this, but it's what we should expect
		from the system, so blah. Would be better IMO to have
		success (within the buffer) and failure (outside the buffer)
		but then we need a way to spell (jump, even outside buffer)

		Should have a test for what to do when we have AppendMatch
		flag in this case...
		"""
		self.failUnlessRaises( TypeError, 
		self.doBasicTest,
			(
				( "ab", Skip, -1, 0 ),
			),
			"abbaab",
			( 1,[
				("ab",0,-1,None),
			],-1),
		)
	
	def testMove1( self ):
		"""Test Move command
		XXX Should have tests for after buffer moves
		"""
		self.doBasicTest(
			(
				( "ab", Move, 4, 0 ),
			),
			"abbaab",
			( 1,[
				("ab",0,4,None),
			],4),
		)
	def testMove2( self ):
		"""Test Move command with negative to middle of buffer
		XXX should have tests for before buffer

		Note: this command is non-intuitive for Python users,
		the negative slicing is 1 beyond what it would be for Python
		(i.e. -1 in Python is 1 before the end, whereas in this
		command it is the end)
		"""
		self.doBasicTest(
			(
				( "ab", Move, -4, 0 ),
			),
			"abbaab",
			( 1,[
				("ab",0,3,None),
			],3),
		)
	def testMove3( self ):
		"""Test Move command
		"""
		self.doBasicTest(
			(
				( "ab", Move, 7, 0 ),
			),
			"abbaab",
			( 1,[
				("ab",0,7,None),
			],7),
		)
	def testMove4( self ):
		"""Test Move to EOF
		"""
		self.doBasicTest(
			(
				( "ab", Move, ToEOF, 0),
			),
			"abbaab",
			( 1,[
				("ab",0,6,None),
			],6),
		)
		
	def testEOF1( self ):
		"""Test EOF command

		Although it's not documented, the original code returned
		the EOF position as the left and right coords for the match,
		so we mimic that behaviour now.
		"""
		self.doBasicTest(
			(
				( "ab", Move, 7, 1 ),
				( "c", EOF, Here, 0 ),
			),
			"abbaab",
			( 1,[
				("ab",0,7,None),
				("c",6,6,None),
			],6),
		)
		
##	def testEOF2( self ):
##		"""Test EOF command when before buffer (can't test this any more, because of new sanity check raising error before we get to check)"""
##		self.doBasicTest(
##			(
##				( "ab", Move, -10, 1 ),
##				( "c", EOF, Here, 0 ),
##			),
##			"abbaab",
##			( 0,[
##			],0),
##		)
	def testEOF3( self ):
		"""Test EOF command when in middle of buffer"""
		self.doBasicTest(
			(
				( "ab", Move, 3, 1 ),
				( "c", EOF, Here, 0 ),
			),
			"abbaab",
			( 0,[
			],AnyInt),
		)
	def testJumpBeforeTable( self ):
		"""Test Jump to before table (explicit fail)

		Note: this reports the position attained by the
		matching child (2) as the "error position", not
		the position before that child (0).
		"""
		self.doBasicTest(
			(
				("ab",Word,"ab",1,-3),
			),
			"abbaab",
			( 0,[
			],AnyInt),
		)
	### tests for ObjectGenerator-idioms
	def testNegativeOptString1( self ):
		"""Negative, optional string value with positive match (should return 0 as length of match)"""
		self.doBasicTest(
			(
				(None, WordEnd, 'test', 2, 1),
				(None, Skip, -4, 2, 2),
				(None, Skip, 1)
			),
			"test",
			(1,[
			],0),
		)
	def testBMSMove( self ):
		"""Negative, optional string value"""
		self.doBasicTest(
			(
				(None, sWordStart, BMS( "cd" ),1,2),
				(None, Move, ToEOF )
			),
			"a",
			(1,[
			],1),
		)
		
	if mxVersion >= ('2','1'):
		def testJumpTargetNamed( self ):
			"""Test JumpTarget command with tagobj specified"""
			self.doBasicTest(
				(
					( "ab", JumpTarget, "SomeString" ),
				),
				"abbaab",
				( 1,[
					("ab",0,0,None),
				],0),
			)
		def testJumpTarget( self ):
			"""Test JumpTarget command in normal usage"""
			self.doBasicTest(
				(
					"this",
				),
				"abbaab",
				( 1,[
				],0),
			)
		

		
def getSuite():
	return unittest.makeSuite(MXSpecialTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
