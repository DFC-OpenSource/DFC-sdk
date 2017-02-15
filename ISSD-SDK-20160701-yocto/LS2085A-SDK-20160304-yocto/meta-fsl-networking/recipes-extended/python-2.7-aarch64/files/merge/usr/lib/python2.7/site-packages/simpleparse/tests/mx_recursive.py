"""Low-level matching tests for mx.TextTools"""
import unittest, pprint
from simpleparse.stt.TextTools import *

ab = (
	( "ab", Word, "ab", 0 ),
)
cdef = (
	( "cd", Word, "cd", 0 ),
	( "ef", Word, "ef", 1,1 ),
)
tableList = [ ab, cdef ]
	

class MXRecursiveTests(unittest.TestCase):
	def doBasicTest(self, table, testvalue, expected, startPosition=0 ):
		result = tag( testvalue, table , startPosition)
		assert result == expected, '''\n\texpected:%s\n\tgot:%s\n'''%( expected, result )

	def testAB( self ):
		"""Test AB testing command"""
		self.doBasicTest(
			ab,
			"abcdef",
			( 1,[
				("ab",0,2,None),
			],2),
		)
	def testCDEF( self ):
		"""Test CDEF testing command"""
		self.doBasicTest(
			cdef,
			"cdef",
			( 1,[
				("cd",0,2,None),
				("ef",2,4,None),
			],4),
		)
	def testABCDEF( self ):
		"""Test abcdef all together"""
		self.doBasicTest(
			ab+cdef,
			"abcdef",
			( 1,[
				("ab",0,2,None),
				("cd",2,4,None),
				("ef",4,6,None),
			],6),
		)
		
	def testTable1( self ):
		"""Test Table command"""
		self.doBasicTest(
			(
				("first", Table, ab),
				("second", Table, cdef),
			),
			"abcdef",
			( 1,[
				("first",0,2,[
					("ab",0,2,None),
				]),
				("second",2,6,[
					("cd",2,4,None),
					("ef",4,6,None),
				]),
			],6),
		)
	def testTableInList1( self ):
		"""Test TableInList command"""
		self.doBasicTest(
			(
				("first", TableInList, (tableList,0)),
				("second", TableInList,(tableList,1)),
			),
			"abcdef",
			( 1,[
				("first",0,2,[
					("ab",0,2,None),
				]),
				("second",2,6,[
					("cd",2,4,None),
					("ef",4,6,None),
				]),
			],6),
		)

	def testSubTable1( self ):
		"""Test SubTable command"""
		self.doBasicTest(
			(
				("first", SubTable, ab),
				("second", SubTable, cdef),
			),
			"abcdef",
			( 1,[
				("ab",0,2,None),
				("first", 0,2, None),
				("cd",2,4,None),
				("ef",4,6,None),
				("second", 2,6, None),
			],6),
		)
	def testSubTable2( self ):
		"""Test SubTable command with no reporting of st groups"""
		self.doBasicTest(
			(
				(None, SubTable, ab),
				(None, SubTable, cdef),
			),
			"abcdef",
			( 1,[
				("ab",0,2,None),
				("cd",2,4,None),
				("ef",4,6,None),
			],6),
		)
	def testSubTableInList1( self ):
		"""Test SubTableInList command"""
		self.doBasicTest(
			(
				("first", SubTableInList, (tableList,0)),
				("second", SubTableInList, (tableList,1)),
			),
			"abcdef",
			( 1,[
				("ab",0,2,None),
				("first", 0,2, None),
				("cd",2,4,None),
				("ef",4,6,None),
				("second", 2,6, None),
			],6),
		)
	def testSubTableNotReturnRecursive( self ):
		"""Test that SubTable calls don't return a recursive structure"""
		result = tag( "abcdef", (
			("first", SubTableInList, (tableList,0)),
			("second", SubTableInList, (tableList,1)),
		), 0)
		assert result [1] is not result[1][1][3], """Subtable results list was the same list as the list enclosing it, looped data structure created"""
			
	def testSubTableInList2( self ):
		"""Test SubTable command with no reporting of st groups"""
		self.doBasicTest(
			(
				(None, SubTableInList, (tableList,0)),
				(None, SubTableInList, (tableList,1)),
			),
			"abcdef",
			( 1,[
				("ab",0,2,None),
				("cd",2,4,None),
				("ef",4,6,None),
			],6),
		)

		
def getSuite():
	return unittest.makeSuite(MXRecursiveTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
