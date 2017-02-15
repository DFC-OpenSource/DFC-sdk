import unittest, pprint, traceback
from simpleparse.parser import Parser
from simpleparse import printers


def rcmp( table1, table2 ):
	"""Silly utility function to get around text search object lack of __cmp__"""
	if len(table1) != len(table2):
		return 0
	else:
		for x,y in map(None, table1, table2):
			if not _rcmp( x,y):
				return 0
		return 1
def _rcmp( item1, item2 ):
	if len(item1) != len(item2):
		return 0
	if item1[1] in (204,):
		if cmp(item1[:2], item2[:2]) != 0:
			return 0
		try:
			if not rcmp( item1[2][0][item1[2][1]], item2[2][0][item2[2][1]]):
				return 0
		except TypeError:
			print item1
			print item2
	elif item1[1] == 207:
		if cmp(item2[:2], item2[:2]) != 0:
			return 0
		if not rcmp( item1[2], item2[2]):
			return 0
	else:
		for a,b in map(None, item1, item2):
			if hasattr(a,'match') and hasattr(b,'match'):
				if not (a.match == b.match and a.translate == b.translate):
					return 0
			elif a != b:
				return 0
	return 1
			
			

class OptimisationTests(unittest.TestCase):
	def testTermCompression( self ):
		"""Test that unreported productions are compressed

		Term compression is basically an inlining of terminal
		expressions into the calling table.  At the moment
		the terminal expressions are all duplicated, which may
		balloon the size of the grammar, not sure if this will
		be an actual problem.  As written, this optimization
		should provide a significant speed up, but there may
		the even more of a speed up if we allow for sharing
		the terminal tuples as well.

		This:
			a:=b <b>:= -c* c:='this'
		Should eventually compress to this:
			a := -'this'*
		"""
		failures = []
		for first, second in [
			("""a:=b <b>:= -c* c:='this'""", """a := -'this'*"""),
			("""a:=b >b<:= c c:= 'this'""", """a := c c:= 'this'"""),
			("""a:=b >b<:= c <c>:= 'this'""", """a := 'this'"""),
			("""a:=b >b<:= c+ <c>:= 'this'""", """a := 'this'+"""),
			# The following will never work, so eventually may raise
			# an error or at least give a warning!
			("""a:=b,c >b<:= c+ <c>:= 'this'""", """a := 'this'+,'this'"""),
			("""a:=b/c >b<:= c+ <c>:= 'this'""", """a := 'this'+/'this'"""),
			# This is requiring group-compression, which isn't yet written
			("""a:=-b/c >b<:= c+ <c>:= 'this'""", """a := -'this'+/'this'"""),
			("""a    := (table1 / table2 / any_line)*
  <any_line> := ANY*, EOL
  <ANY>      := -EOL
  <EOL>      := '\n'
  table1 := 'a'
  table2 := 'b'
  """, """a    := (table1 / table2 / (-'\n'*, '\n'))*
    table1 := 'a'
  table2 := 'b'
"""),
			("""a:= b,c <b>:= -c* <c>:= '\n'""", """a := -'\n'*,'\n'"""),
			
		]:
			pFirst = Parser( first, "a")
			pSecond = Parser( second, "a")
			tFirst = pFirst.buildTagger()
			tSecond = pSecond.buildTagger()
			if not rcmp( tFirst , tSecond):
				tFirstRepr = pprint.pformat(tFirst)
				tSecondRepr = pprint.pformat(tSecond)
				failures.append( """%(first)r did not produce the same parser as %(second)r\n\t%(tFirstRepr)s\n\t%(tSecondRepr)s"""%locals())
		if failures:
			raise ValueError( "\n".join(failures))
	def testTermSharing( self ):
		"""Test that shared terminal productions are using the same parser"""
		first =""" a := b,b >b<:= d d:= 'this'"""
		pFirst = Parser( first, "a")
		tFirst = pFirst.buildTagger()
		b,c = tFirst
		assert b is c, """Not sharing the same tuple for b and c instances"""
	def testNoReportPassDown( self ):
		"""Test that a non-reporting production does not produce reporting sub-productions"""
		first =""" a := b <b>:= d,e d:= e e:= 'this'"""
		second =""" a := 'this' """
		assert Parser( first, 'a').parse( 'thisthis' ) == (1,[
		],8)
		
	def testNameCollapseForSimple( self ):
		"""Test that a name reference, given a single-item reporting avoids extra table"""
		first =""" a := b,b b:= 'this'"""
		# The result should be...
		expected = ( ('b',21,'this'),('b',21,'this'))
		table = Parser( first, 'a').buildTagger( )
		assert table == expected, "%s != %s"%(
			pprint.pformat( table),
			pprint.pformat(expected),
		)
		
def getSuite():
	return unittest.makeSuite(OptimisationTests,'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
