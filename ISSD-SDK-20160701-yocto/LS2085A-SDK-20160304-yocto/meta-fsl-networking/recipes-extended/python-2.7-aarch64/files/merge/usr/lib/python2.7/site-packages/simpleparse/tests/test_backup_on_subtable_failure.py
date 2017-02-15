declaration = r'''testparser := (a,b)*
a := 'a'
b := 'b'
'''
testdata = 'aba'
expectedResult = (1, [('a',0,1,[]), ('b',1,2,[])], 2 )

from simpleparse.simpleparsegrammar import Parser
from simpleparse.stt.TextTools import TextTools
import pprint


parser = Parser( declaration ).generator.buildParser('testparser' )
result = TextTools.tag( testdata, parser )
if result != expectedResult:
	print 'backup-on-subtable-test failed'
	print '\texpected', pprint.pprint( expectedResult )
	print '\tgot', pprint.pprint( result )

