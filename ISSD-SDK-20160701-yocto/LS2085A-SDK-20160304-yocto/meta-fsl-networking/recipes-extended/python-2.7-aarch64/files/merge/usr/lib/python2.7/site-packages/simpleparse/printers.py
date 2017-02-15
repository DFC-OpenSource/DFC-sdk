"""Utility to print Python code for a given generator object's element tokens"""
import string
class _GeneratorFormatter:
	"""Singleton Class to give a generator's element tokens as a source string

	Call this as:
		printers.asGenerator( generator ) to get a Python source string
		that tries to recreate the generator as a set of objectgenerator
		element token objects (as seen in simpleparsegrammar).
	"""
	HEAD = """from simpleparse import generator
from simpleparse.objectgenerator import *
GENERATOR = generator.Generator ()

class Parser:
	'''Mix-in class for simpleparse.parser.Parser which uses this GENERATOR
	to build tagging tables.  You'll likely want to override __init__ to avoid
	building a new parser from a grammar (or subclass BaseParser instead of
	Parser)
	'''
	def buildTagger( self, name=None, processor = None ):
		'''Build the tag-table for parsing the EBNF for this parser'''
		return GENERATOR.buildParser( name, processor )

"""
	ITEM = """GENERATOR.addDefinition(
    %(name)s,
    %(element)s,
)
"""
	def __call__( self, generator ):
		temp = [self.HEAD]
		for name,element in map(None, generator.getNames(), generator.getRootObjects()):
			name = repr(name)
			element = self.reprObject(element,1)
			temp.append( self.ITEM%locals())
		return string.join( temp, "")
	def reprObject( self, obj, depth=0, indent='    ' ):
		"""Return a recognisable version of an objectgenerator element token"""
		argTemplate = (indent*(depth+1))+"%s = %s,"
		temp = ["""%s("""%(obj.__class__.__name__)]
		for key,value in obj.__dict__.items():
			if key == 'children':
				childTemplate = (indent*(depth+2)) + '%s,'
				childTemp = ["["]
				for child in value:
					childTemp.append(childTemplate%self.reprObject(child,depth+2))
				childTemp.append( (indent*(depth+1))+']' )
				
				temp.append(
					argTemplate% (key, string.join(childTemp, '\n'))
				)
			else:
				temp.append( argTemplate%( key, repr(value)))
		temp.append( (indent*depth)+')')
		return string.join( temp,'\n')

asGenerator = _GeneratorFormatter()
asObject = asGenerator.reprObject

