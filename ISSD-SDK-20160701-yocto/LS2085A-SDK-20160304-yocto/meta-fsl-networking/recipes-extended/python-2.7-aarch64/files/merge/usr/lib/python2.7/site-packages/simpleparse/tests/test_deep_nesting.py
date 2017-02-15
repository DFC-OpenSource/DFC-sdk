from simpleparse.simpleparsegrammar import Parser
from simpleparse.stt.TextTools import TextTools
import pprint
from genericvalues import NullResult, AnyInt


declaration = r'''testparser := as?
as := a,as?
a := 'a'
'''
testdata = 'aaaa'
expectedResult = (1, [
	('as', 0, 4, [
		('a', 0, 1, NullResult),
		('as', 1, 4, [
			('a', 1, 2, NullResult),
			('as', 2, 4, [
				('a', 2, 3, NullResult),
				('as', 3, 4, [
					('a', 3, 4, NullResult)
				])
			])
		])
	])
], 4)


parser = Parser( declaration ).generator.buildParser( 'testparser' )
print "About to attempt the deep-nesting test"
print "If python goes into an infinite loop, then the test failed ;) "
print
result = TextTools.tag( testdata, parser )
if result != expectedResult:
	print 'test-deep-nesting failed'
	print '\texpected', expectedResult
	print '\tgot', result 
else:
	print "test-deep-nesting succeeded!\nYou're probably using the non-recursive mx.TextTools rewrite"
