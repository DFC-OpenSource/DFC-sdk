import unittest
import mx_test, test_objectgenerator, test_simpleparsegrammar
import test_common_chartypes, test_common_numbers
import sys
try:
	import test_common_iso_date
except ImportError:
	sys.stderr.write( """Unable to test ISO dates, no mxDateTime module\n""" )
	test_common_iso_date = None
import test_common_strings, test_printers, test_optimisation, test_common_comments
import test_xml

import string
from simpleparse.stt import TextTools
mxVersion = tuple(string.split( TextTools.__version__, '.'))
mxVersion = mxVersion[:3]

def getSuite():
	set = []
	for module in [
		mx_test,
		test_objectgenerator,
		test_simpleparsegrammar,
		test_common_chartypes,
		test_common_numbers,
		test_common_iso_date,
		test_common_strings,
		test_common_comments,
		test_printers,
		test_xml,
		test_optimisation,
	]:
		if hasattr( module, 'getSuite'):
			set.append( module.getSuite() )
		elif module:
			sys.stderr.write( "module %r has no getSuite function, skipping\n"%(module,))
	return unittest.TestSuite(
		set
	)

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
