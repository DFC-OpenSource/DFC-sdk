import unittest, pprint, traceback
from simpleparse.objectgenerator import *
from simpleparse.stt.TextTools import TextTools
from genericvalues import NullResult, AnyInt

class ElementTokenTests(unittest.TestCase):
	def doBasicTest(self, instance, testvalue, expected, startPosition=0 ):
		table = tuple(instance.toParser())
		result = tag( testvalue, table , startPosition)
		assert result == expected, '''\n\texpected:%s\n\tgot:%s\n'''%( expected, result )
	def testString1( self ):
		self.doBasicTest(
			Literal( value = 'test' ),
			'test',
			(1, [],4),
		)
	def testString2( self ):
		self.doBasicTest(
			Literal( value = 'test', optional =1 ),
			'test',
			(1, [],4),
		)
	def testString3( self ):
		self.doBasicTest(
			Literal( value = 'test', optional =1, negative=1 ),
			'test',
			(1, [],0),
		)
	def testString4( self ):
		self.doBasicTest(
			Literal( value = 'test', negative=1 ),
			'test',
			(0, [],AnyInt),
		)
	def testString5( self ):
		self.doBasicTest(
			Literal( value = 'test', repeating=1),
			'testtest',
			(1, [],8),
		)
	def testString6( self ):
		self.doBasicTest(
			Literal( value = 'test', repeating=1, optional = 1),
			'testtest',
			(1, [],8),
		)
	def testString7( self ):
		self.doBasicTest(
			Literal( value = 'test', repeating=1, optional = 1, negative = 1),
			'testtest',
			(1, [],0),
		)
	def testString8( self ):
		"""Test repeating negative string"""
		self.doBasicTest(
			Literal( value = 'test', repeating=1, negative = 1),
			'testtest',
			(0, [],AnyInt),
		)
	def testString9( self ):
		self.doBasicTest(
			Literal( value = '\\',),
			'\\',
			(1, [],1),
		)
	def testRange1( self ):
		self.doBasicTest(
			Range( value = 'abc'),
			'aabbcc',
			(1, [],1),
		)
	def testRange2( self ):
		self.doBasicTest(
			Range( value = 'abc', optional=1),
			'aabbcc',
			(1, [],1),
		)
	def testRange3( self ):
		self.doBasicTest(
			Range( value = 'abc', optional=1, repeating=1),
			'aabbcc',
			(1, [],6),
		)
	def testRange4( self ):
		self.doBasicTest(
			Range( value = 'abc', optional=1, repeating=1, negative=1),
			'aabbcc',
			(1, [],0),
		)
	def testRange5( self ):
		self.doBasicTest(
			Range( value = 'abc', optional=1, negative=1),
			'aabbcc',
			(1, [],0),
		)
	def testRange6( self ):
		self.doBasicTest(
			Range( value = 'abc', negative=1),
			'aabbcc',
			(0, [],AnyInt),
		)
	def testRange7( self ):
		self.doBasicTest(
			Range( value = 'abc', negative=1, repeating=1),
			'aabbcc',
			(0, [],AnyInt),
		)
	def testRange8( self ):
		self.doBasicTest(
			Range( value = 'abc', negative=1, repeating=1),
			'defc',
			(1, [],3),
		)
	def testRange9( self ):
		self.doBasicTest(
			Range( value = 'abc', negative=1),
			'defc',
			(1, [],1),
		)
	def testSequential1( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=0,
			),
			'atest',
			(1, [],5),
		)
	def testSequential2( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=1,
			),
			'atest',
			(0, [],AnyInt),
		)
	def testSequential3( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=1, optional=1,
			),
			'atest',
			(1, [],0),
		)
	def testSequential4( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=1, optional=1, repeating=1,
			),
			'sdatest',
			(1, [],2),
		)
	def testSequential5( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				optional=1, repeating=1,
			),
			'atestbtestctest',
			(1, [],15),
		)
	def testSequential6( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				optional=1, 
			),
			'atestbtestctest',
			(1, [],5),
		)
		
	def testSequential7( self ):
		self.doBasicTest(
			SequentialGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				optional=1, 
			),
			'satestbtestctest',
			(1, [],0),
		)


	def testFirstOf1( self ):
		self.doBasicTest(
			FirstOfGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=0,
			),
			'atest',
			(1, [],1),
		)
	def testFirstOf2( self ):
		self.doBasicTest(
			FirstOfGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=0,
			),
			'testa',
			(1, [],4),
		)
	def testFirstOf3( self ):
		self.doBasicTest(
			FirstOfGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=1,
			),
			'testa',
			(0, [],AnyInt),
		)
	def testFirstOf4( self ):
		self.doBasicTest(
			FirstOfGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				negative=1, optional=1,
			),
			'testa',
			(1, [],0),
		)
	def testFirstOf5( self ):
		self.doBasicTest(
			FirstOfGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				repeating=1,
			),
			'testabtest',
			(1, [],10),
		)
	def testFirstOf6( self ):
		self.doBasicTest(
			FirstOfGroup(
				children = [
					Range( value = 'abc',),
					Literal( value = 'test', ),
				],
				repeating=1, negative = 1,
			),
			'hellotheretestabtest',
			(1, [],10),
		)

	def testCIString1( self ):
		self.doBasicTest(
			CILiteral( value = 'test'),
			'test',
			(1, [],4),
		)
	def testCIString2( self ):
		self.doBasicTest(
			CILiteral( value = 'test'),
			'Test',
			(1, [],4),
		)
	def testCIString3( self ):
		self.doBasicTest(
			CILiteral( value = 'test'),
			'TEST',
			(1, [],4),
		)
	def testCIString4( self ):
		self.doBasicTest(
			CILiteral( value = 'test'),
			'tes',
			(0, [],AnyInt),
		)
	def testCIString5( self ):
		self.doBasicTest(
			CILiteral( value = 'test', optional=1),
			'tes',
			(1, [], 0),
		)

### Simpleparse 2.0.0b4 introduced an explicit check that
##  rejects FOGroups with optional children to prevent
##  infinite recursions

##	def testFirstOf7( self ):
##		'''([abc]?/"test"?)*
##
##		Demonstrates a recently fixed error, namely a fix to the repeating
##		code which explicitly checks for EOF condition during repeating
##		loops.  Result is that this condition should be handled correctly.
##
##		Old Note:
##			This test exposes a problem with both the original generator
##			and the sub-class here.  FOGroups with optional children are
##			in danger of never returning as the children always "succeed"
##			even if they consume nothing.
##			Failure in this case is likely to be an endless loop, so we
##			can expect that if this is broken there will be heck to pay ;)
##		'''
##		generator =	FirstOfGroup(
##				children = [
##					Range( value = 'abc', optional=1),
##					Literal( value = 'test', optional=1),
##				],
##				repeating=1, optional=1,
##			)
##		self.doBasicTest(
##			generator,
##			'testabtest',
##			(1, [],10),
##		)
##		generator =	FirstOfGroup(
##				children = [
##					Range( value = 'abc', optional=1),
##					Literal( value = 'test', optional=1),
##					SequentialGroup(
##						children = [
##							Literal( value = 'm', optional=1),
##							Literal( value = 'n', optional=1),
##						],
##					),
##				],
##				repeating=1, optional=1,
##			)
##		self.doBasicTest(
##			generator,
##			'testmnabtest',
##			(1, [],12),
##		)
		
	def testNegative1( self ):
		self.doBasicTest(
			Literal( value = 's', negative=1),
			's\\',
			(0, [],AnyInt),
		)
	def testNegative2( self ):
		self.doBasicTest(
			Literal( value = 's', negative=1),
			'asa\\',
			(1, [],1),
		)
	def testNegative3( self ):
		self.doBasicTest(
			Literal( value = 's', negative=1, repeating=1),
			'aasa\\',
			(1, [],2),
		)
	def testNegative4( self ):
		self.doBasicTest(
			Literal( value = 's', negative=1, repeating=1, optional=1),
			'a',
			(1, [],1),
		)
	def testNegative4a( self ):
		self.doBasicTest(
			Literal( value = 's', negative=1, repeating=1, optional=1),
			'as',
			(1, [],1),
		)
	def testNegative4b( self ):
		self.doBasicTest(
			Literal( value = 's', negative=1, repeating=1, optional=1),
			'sas',
			(1, [],0),
		)
	def testNegative5( self ):
		self.doBasicTest(
			Range( value = 'sat', negative=1),
			'aasat\\',
			(0, [],AnyInt), 
		)
	def testNegative6( self ):
		self.doBasicTest(
			Range( value = 'sat', negative=1, repeating=1),
			'aasat\\',
			(0, [],AnyInt), 
		)
	def testNegative7( self ):
		self.doBasicTest(
			Range( value = 'sat', negative=1, repeating=1, optional=1),
			'aasat\\',
			(1, [],0), 
		)
		
def getSuite():
	return unittest.makeSuite(ElementTokenTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
