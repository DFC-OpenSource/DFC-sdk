import unittest, pprint
from simpleparse.parser import Parser
from simpleparse.stt.TextTools import TextTools
from genericvalues import NullResult, AnyInt
		
class ParserGenerationTests(unittest.TestCase):
	def doBasicTest(self, definition, parserName, testValue, expected, ):
		result = Parser( definition).parse( testValue, parserName )
		assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )
	def testGenNegRange1( self ):
		self.doBasicTest(
			'''s := - something *
			<something> := [ab]''',
			's',
			'mmmab',
			(1,[],3)
		)
	def testGenNegRange2( self ):
		self.doBasicTest(
			'''s := - something
			<something> := [ab]''',
			's',
			'mmmab',
			(1,[],1)
		)
	def testGenNegLit1( self ):
		self.doBasicTest(
			'''s := - something *
			<something> := "a"''',
			's',
			'mmmab',
			(1,[],3)
		)
	def testGenPosReptOpt1( self ):
		self.doBasicTest(
			'''s := something *
			something := "a" ''',
			's',
			'aammmab',
			(1,[("something",0,1,NullResult),("something",1,2,NullResult)],2)
		)
	def testGenPosReptOpt2( self ):
		self.doBasicTest(
			'''s := something *
			something := "a" ''',
			's',
			'mmmab',
			(1,[],0)
		)
	def testGenPosRept1( self ):
		self.doBasicTest(
			'''s := something +
			something := "a" ''',
			's',
			'mmmab',
			(0,[],AnyInt)
		)

	def testLookaheadPositive( self ):
		self.doBasicTest(
			'''s := ?"b"
			''',
			's',
			'bbbba',
			(1,[
			],0)
		)
	def testLookaheadNeg( self ):
		self.doBasicTest(
			'''s := ?-"b"
			''',
			's',
			'bbbba',
			(0,[
			],AnyInt)
		)
	def testLookaheadNeg2( self ):
		self.doBasicTest(
			'''s := ?-"b"?
			''',
			's',
			'bbbba',
			(1,[
			],0)
		)
	def testLookaheadNeg3( self ):
		self.doBasicTest(
			'''s := "b", ?-"a"
			''',
			's',
			'bbbba',
			(1,[
			],1)
		)
	def testLookaheadNeg4( self ):
		self.doBasicTest(
			'''s := "b", ?-"a", "ba"
			''',
			's',
			'bba',
			(1,[
			],3)
		)
	def testLookaheadNeg5( self ):
		self.doBasicTest(
			'''s := ?-t, "ba"
			t := "bad"
			''',
			's',
			'bac',
			(1,[
			],2)
		)
	def testLookaheadNeg6( self ):
		self.doBasicTest(
			'''s := ?-t, "ba"
			t := "bad"
			''',
			's',
			'bad',
			(0,[
			],AnyInt)
		)
	def testLookahead2( self ):
		"""Test lookahead on literals (more complex)"""
		self.doBasicTest(
			'''s := something+, "ba"
			something := "b",?-"a"
			''',
			's',
			'bbbba',
			(1,[
				("something",0,1,NullResult),
				("something",1,2,NullResult),
				("something",2,3,NullResult),
			],5)
		)
	def testLookahead3( self ):
		"""Test lookahead on reported positive productions"""
		self.doBasicTest(
			'''s := ?trailer
			trailer   := "bad"
			''',
			's',
			'badba',
			(1,[
				("trailer",0,3,NullResult),
			],0)
		)
	def testLookahead4( self ):
		self.doBasicTest(
			'''s := ?-trailer?
			trailer   := "bad"
			''',
			's',
			'badba',
			(1,[
			],0)
		)

	def testLookahead5( self ):
		self.doBasicTest(
			'''s := ?-trailer, 'ba'
			trailer   := "bad"
			''',
			's',
			'babba',
			(1,[
			],2)
		)
	def testLookahead6( self ):
		self.doBasicTest(
			'''s := ?-trailer, 'ba'
			trailer   := "bad"
			''',
			's',
			'badba',
			(0,[
			],AnyInt)
		)

	def testGenPos1( self ):
		self.doBasicTest(
			'''s := something
			something := "a" ''',
			's',
			'mmmab',
			(0,[],AnyInt)
		)
	def testGenPos2( self ):
		self.doBasicTest(
			'''s := something
			something := "a" ''',
			's',
			'ammmab',
			(1,[('something',0,1,NullResult),],1)
		)

	def testOptionalGroupHitEOF( self ):
		"""Test optional group hitting an EOF during success run"""
		self.doBasicTest(
			'''s := something*
			something := ("a"/"b") ''',
			's',
			'aa',
			(1,[
				('something',0,1,NullResult),
				('something',1,2,NullResult),
			],2)
		)
	def testMultiLineDef( self ):
		"""Test multi-line definitions"""
		self.doBasicTest(
			'''s :=
			something*
			something := (
				"a"/
				"b"
			) ''',
			's',
			'aa',
			(1,[
				('something',0,1,NullResult),
				('something',1,2,NullResult),
			],2)
		)
##	def testRepeatOptionalFail( self ):
##		"""Explicit test of the optional-repeating-child of repeating object
##		"""
##		self.doBasicTest(
##			r'''
##			controlword := '\\',('*','\\')?,[-a-zA-Z0-9]+
##			contents := -[\012}\\]*
##			file := (controlword/contents)+
##			''',
##			"file",
##			"\\*\\test sdf ff f f sdfff\\",
##			(1, [
##				("controlword", 0,7,[]),
##				("contents",7,24),
##			],24),
##		)

	def testGenCILiteral1( self ):
		self.doBasicTest(
			'''s := c"this"''',
			's',
			'this',
			(1,[],4)
		)
	def testGenCILiteral2( self ):
		self.doBasicTest(
			'''s := c"this"''',
			's',
			'This',
			(1,[],4)
		)
	def testGenCILiteral3( self ):
		self.doBasicTest(
			'''s := c"this"''',
			's',
			'THIS',
			(1,[],4)
		)
	def testGenCILiteral4( self ):
		self.doBasicTest(
			'''s := -c"this"''',
			's',
			' THIS',
			(1,[],1)
		)
	def testGenCILiteral5( self ):
		self.doBasicTest(
			'''s := -c"this"''',
			's',
			' thi',
			(1,[],1)
		)
	def testGenCILiteral6( self ):
		self.doBasicTest(
			'''s := -c"this"*''',
			's',
			' thi',
			(1,[],4)
		)

class NameTests(unittest.TestCase):
	def doBasicTest(self, definition, parserName, testValue, expected, ):
		result = Parser( definition).parse( testValue, production=parserName )
		assert result == expected, '''\nexpected:%s\n     got:%s\n'''%( expected, result )
	def test_p( self ):
		self.doBasicTest(
			'''s := something
			something := "a" ''',
			's',
			'ammmab',
			(1,[('something',0,1,NullResult),],1)
		)
	def test_po( self ):
		self.doBasicTest(
			'''s := something?
			something := "a" ''',
			's',
			'ammmab',
			(1,[('something',0,1,NullResult),],1)
		)
	def test_por( self ):
		self.doBasicTest(
			'''s := something*
			something := "a" ''',
			's',
			'ammmab',
			(1,[('something',0,1,NullResult),],1)
		)
	def test_pr( self ):
		self.doBasicTest(
			'''s := something+
			something := "a" ''',
			's',
			'ammmab',
			(1,[('something',0,1,NullResult),],1)
		)

	def test_n( self ):
		self.doBasicTest(
			'''s := - something
			<something> := [ab]''',
			's',
			'mmmab',
			(1,[],1)
		)
	def test_no( self ):
		self.doBasicTest(
			'''s := - something?
			<something> := [ab]''',
			's',
			'mmmab',
			(1,[],1)
		)
	def test_nor( self ):
		self.doBasicTest(
			'''s := - something*
			<something> := [ab]''',
			's',
			'mmmab',
			(1,[],3)
		)
	def test_nr( self ):
		self.doBasicTest(
			'''s := - something+
			<something> := [ab]''',
			's',
			'mmmab',
			(1,[],3)
		)
	def test_n_f( self ):
		self.doBasicTest(
			'''s := - something
			<something> := [ab]''',
			's',
			'ammmab',
			(0,[],AnyInt)
		)
	def test_no_f( self ):
		self.doBasicTest(
			'''s := - something?
			<something> := [ab]''',
			's',
			'ammmab',
			(1,[],0)
		)
	def test_nor_f( self ):
		self.doBasicTest(
			'''s := - something*
			<something> := [ab]''',
			's',
			'ammmab',
			(1,[],0)
		)
	def test_nr_f( self ):
		self.doBasicTest(
			'''s := - something +
			<something> := [ab]''',
			's',
			'ammmab',
			(0,[],AnyInt)
		)
##	def test_por_big( self ):
##		"""This test creates 1,000,000 result tuples (very inefficiently, I might add)...
##		on my machine that takes a long time, so I do not bother with the test
##		(note that with a recursive mx.TextTools, this should actually blow up
##		 long before you get into memory problems :) ).
##		"""
##		self.doBasicTest(
##			'''s := something*
##			something := "a" ''',
##			's',
##			'a'*1000000,
##			(1,[
##			],1000000)
##		)

	def test_expanded_name( self ):
		"""Non-reporting (expanded) name test

		Tests new feature, a name whose children
		are reported, but which is not itself reported,
		basically this lets you create anonymous
		groups which can be referenced from other
		productions.
		"""
		self.doBasicTest(
			'''s := something +
			>something< := r
			r := [ab]
			v := [c]
			''',
			's',
			'abammmab',
			(1,[
				('r',0,1, NullResult),
				('r',1,2, NullResult),
				('r',2,3, NullResult),
			],3)
		)
		
	def test_expanded_SingleNameChild( self ):
		"""Expanded group with single child which is a Name itself

		This originally failed when the Name object's report value
		was changed to 0 (redundant information for the "expanded" code),
		resulting in the child production not getting reported.
		"""
		self.doBasicTest(
			'''s := something +
			something := r
			r := [ab]''',
			'something',
			'abammmab',
			(1,[
				('r',0,1, NullResult),
			],1)
		)

class BasicMethodSource:
	def __init__( self ):
		self.results = []
	def _m_a( self, taglist,text,l,r,subtags ):
		self.results.append( ('a',text[l:r]))
	def _m_b( self, taglist, text, l,r,subtags):
		self.results.append( ('b',l,r) )
	_m_c = TextTools.AppendMatch
	_m_d = TextTools.AppendTagobj
	_o_d = "hello world"
class AppendToTagobjMethodSource:
	def __init__( self ):
		self._o_d = []
	_m_d = TextTools.AppendToTagobj

class CallTests(unittest.TestCase):
	"""Tests semantics of calling objects from a method source during parsing"""
	def parse( self, definition, parserName, testValue, source):
		result = Parser(
			definition,
		).parse(testValue, production=parserName, processor = source)
		return result
	def test_basic_call( self ):
		"""Test basic ability to call a method instead of regular functioning"""
		source = BasicMethodSource()
		self.parse( """
			x := (a/b)*
			a := "a"
			b := "b"
		""", 'x', 'abba', source)
		assert source.results == [ ('a','a'),('b',1,2),('b',2,3),('a','a'),], """Method source methods were not called, or called improperly:\n%s"""%(source.results,)
		
	def test_AppendMatch( self ):
		"""Test ability to append the text-string match to the results list"""
		source = BasicMethodSource()
		result = self.parse( """
			x := c*
			c := 'c'
		""", 'x', 'ccc', source)
		assert result == (1,[
			'c','c','c',
		],3), """Result was %s"""%( result, )
		
	def test_AppendTagObj( self ):
		"""Test appending the tagobject to the results list"""
		source = BasicMethodSource()
		result = self.parse( """
			x := d*
			d := 'd'
		""", 'x', 'ddd', source)
		assert result == (1,[
			"hello world","hello world","hello world",
		],3)

	def test_AppendToTagObj( self ):
		"""Test basic ability to call a method instead of regular functioning"""
		source = AppendToTagobjMethodSource()
		result = self.parse( """
			x := d*
			d := 'd'
		""", 'x', 'ddd', source)
		assert source._o_d == [ (None,0,1,NullResult),(None,1,2,NullResult),(None,2,3,NullResult)], """Method source methods were not called, or called improperly:\n%s"""%(source._o_d,)

import test_grammarparser
import test_erroronfail

def getSuite():
	return unittest.TestSuite((
		test_grammarparser.getSuite(),
		test_erroronfail.getSuite(),
		unittest.makeSuite(ParserGenerationTests, 'test'),
		unittest.makeSuite(NameTests, 'test'),
		unittest.makeSuite(CallTests, 'test'),
	))

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
