import unittest, pprint
from simpleparse.stt.TextTools import *
import string
from simpleparse.stt import TextTools
mxVersion = tuple(string.split( TextTools.__version__, '.')[:3])

class MXFlagTests(unittest.TestCase):
	"""Test Flags for returning/calling different functions on success"""
	def doBasicTest(self, table, testvalue, expected, startPosition=0 ):
		result = tag( testvalue, table , startPosition)
		assert result == expected, '''\n\texpected:%s\n\tgot:%s\n'''%( expected, result )
	### Return-type handling tests...
	def testCallTag1( self ):
		"""Test CallTag"""
		def function (parentList, text, l,r,children):
			parentList.append( (text[l:r], children) )
		self.doBasicTest(
			(
				( function, AllIn + CallTag, "ab", 0 ),
			),
			"abbaabccd",
			( 1,[
				("abbaab",None),
			],6),
		)
	def testCallTag2( self ):
		"""Test CallTag with a class instance"""
		class A:
			def __call__(self, parentList, text, l,r,children):
				parentList.append( (text[l:r], children) )
		self.doBasicTest(
			(
				( A(), AllIn + CallTag, "ab", 0 ),
			),
			"abbaabccd",
			( 1,[
				("abbaab",None),
			],6),
		)
	def testAppendMatch1( self ):
		"""Test AppendMatch"""
		def function (parentList, text, l,r,children):
			parentList.append( (text[l:r], children) )
		self.doBasicTest(
			(
				( function, AllIn + AppendMatch, "ab", 0 ),
			),
			"abbaabccd",
			( 1,[
				"abbaab",
			],6),
		)
	def testAppendToTagobj1( self ):
		"""Test AppendToTagobj"""
		class X:
			successful = ""
			def append(self, value):
				self.successful = value
		tag = X()
		self.doBasicTest(
			(
				( tag, AllIn + AppendToTagobj, "ab", 0 ),
			),
			"abbaabccd",
			( 1,[
			],6),
		)
		assert tag.successful == (None,0,6,None), "TagObject's append was called with %s"%(repr(tag.successful),)
	def testAppendToTagobj2( self ):
		"""Test AppendToTagobj with a simple list"""
		
		tag = []
		self.doBasicTest(
			(
				( tag, AllIn + AppendToTagobj, "ab", 0 ),
			),
			"abbaabccd",
			( 1,[
			],6),
		)
		assert tag[0] == (None,0,6,None), "TagObject's append was called with %s"%(repr(tag.successful),)
		
	def testAppendTagobj1( self ):
		"""Test AppendTagobj"""
		self.doBasicTest(
			(
				( "Hi there world!", AllIn + AppendTagobj, "ab", 0 ),
			),
			"abbaabccd",
			( 1,[
				"Hi there world!",
			],6),
		)
	if mxVersion >= ('2','1'):
		def testLookAhead1(  self ):
			"""Test LookAhead"""
			self.doBasicTest(
				(
					( "whatever", AllIn + LookAhead, "ab", 0 ),
				),
				"abbaabccd",
				( 1,[
					("whatever",0,6,None),
				],0),
			)
		def testLookAhead2(  self ):
			"""Test LookAhead"""
			self.doBasicTest(
				(
					( "whatever", AllIn + LookAhead, "ab", 0 ),
					( "whatever2", AllIn, "ab", 0 ),
				),
				"abbaabccd",
				( 1,[
					("whatever",0,6,None),
					("whatever2",0,6,None),
				],6),
			)



def getSuite():
	return unittest.makeSuite(MXFlagTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
