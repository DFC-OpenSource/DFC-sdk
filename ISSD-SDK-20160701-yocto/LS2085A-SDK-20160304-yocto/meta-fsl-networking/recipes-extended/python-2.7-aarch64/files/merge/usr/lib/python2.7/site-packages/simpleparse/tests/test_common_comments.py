"""Test the various common library comment productions"""
import unittest, string
from simpleparse.parser import Parser
from simpleparse.common import comments
from simpleparse import dispatchprocessor


parseTests = [
	# each production should match the whole of all of the first,
	# and not match any of the second...
	("c_comment", [
		"""/* this */""",
		"""/* this \n\n*/""",
	],[
		"""// this""",
		"""# this""",
		"""# this\n""",
		"""# this\r\n""",
	]),
	("c_nest_comment", [
		"""/* this */""",
		"""/* this \n\n*/""",
		"""/* /* this */ */""",
		"""/* /* this \n*/ */""",
	],[
		"""// this""",
		"""# this""",
		"""; this""",
	]),
	("hash_comment", [
		"""# this""",
		"""# this\n""",
		"""# this\r\n""",
	],[
		"""// this""",
		"""/* this */""",
		"""/* /* this */ */""",
	]),
	("semicolon_comment", [
		"""; this""",
		"""; this\n""",
		"""; this\r\n""",
	],[
		"""# this""",
		"""// this""",
		"""/* this */""",
		"""/* /* this */ */""",
	]),
	("slashslash_comment", [
		"""// this""",
		"""// this\n""",
		"""// this\r\n""",
	],[
		"""# this""",
		"""/ this""",
		"""/* this */""",
		"""/* /* this */ */""",
	]),
]


class CommonTests(unittest.TestCase):
	def testBasic( self ):
		for production, yestable, notable in parseTests:
			p = Parser( "x := %s"%production, 'x')
			for data in yestable:
				success, results, next = p.parse( data)
				assert success and (next == len(data)), """Did not parse comment %s as a %s result=%s"""%( repr(data), production, (success, results, next))
				assert results, """Didn't get any results for comment %s as a %s result=%s"""%( repr(data), production, (success, results, next))
			for data in notable:
				success, results, next = p.parse( data)
				assert not success, """Parsed %s of %s as a %s result=%s"""%( 
					next, repr(data), production, results
				)
		
def getSuite():
	return unittest.makeSuite(CommonTests, 'test')

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
