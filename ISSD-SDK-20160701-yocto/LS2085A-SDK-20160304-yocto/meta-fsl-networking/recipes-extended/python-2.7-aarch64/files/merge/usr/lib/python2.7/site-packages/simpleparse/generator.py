"""Abstract representation of an in-memory grammar that generates parsers"""
from simpleparse.stt.TextTools import TextTools
import traceback

class Generator:
	'''Abstract representation of an in-memory grammar that generates parsers
	
	The generator class manages a collection of
	ElementToken objects.  These element token objects
	allow the generator to be separated from the
	particular parser associated with any particular EBNF
	grammar.  In fact, it is possible to create entire grammars
	using only the generator objects as a python API.
	'''
	def __init__( self ):
		"""Initialise the Generator"""
		self.names = []
		self.rootObjects = []
		self.methodSource = None
		self.definitionSources = []
	def getNameIndex( self, name ):
		'''Return the index into the main list for the given name'''
		try:
			return self.names.index( name )
		except ValueError:
			
			for source in self.definitionSources:
				if source.has_key( name ):
					return self.addDefinition( name, source[name])
##			import pdb
##			pdb.set_trace()
			raise NameError( '''The name %s is not defined within this generator'''%(repr(name)), self )
	def getRootObjects( self, ):
		'''Return the list of root generator objects'''
		return self.rootObjects
	def getNames( self, ):
		'''Return the list of root generator objects'''
		return self.names
	def getRootObject( self, name ):
		"""Get a particular root object by name"""
		return self.getRootObjects()[ self.getNameIndex(name)]
	
	def addDefinition( self, name, rootElement ):
		'''Add a new definition (object) to the generator'''
		try:
			self.names.index( name )
			raise NameError( '''Attempt to redefine an existing name %s'''%(name), self )
		except ValueError:
			self.names.append( name )
			self.rootObjects.append( rootElement )
			return self.getNameIndex( name )
	def buildParser( self, name, methodSource=None ):
		'''Build the given parser definition, returning a TextTools parsing tuple'''
		self.parserList = []
		self.terminalParserCache = {}
		self.methodSource = methodSource
		i = 0
		while i < len(self.rootObjects):
			# XXX Note: rootObjects will grow in certain cases where
			# a grammar is loading secondary grammars into itself
			rootObject = self.rootObjects[i]
			try:
				if len(self.parserList) <= i or self.parserList[i] is None:
					parser = tuple(rootObject.toParser( self ))
					self.setTerminalParser( i, parser )
			except NameError,err:
				currentRuleName = self.names[i]
				err.args = err.args + ('current declaration is %s'%(currentRuleName), )
				raise
			i = i + 1
		assert None not in self.parserList, str( self.parserList)
		return self.parserList [self.getNameIndex (name)]
	def setTerminalParser( self, index, parser ):
		"""Explicitly set the parser value for given name"""
		while index >= len(self.parserList):
			self.parserList.append(None)
		self.parserList[index] = parser
	def getTerminalParser( self, index ):
		"""Try to retrieve a parser from the parser-list"""
		try:
			return self.parserList[ index ]
		except IndexError:
			return None
	def cacheCustomTerminalParser( self, index, flags, parser ):
		"""Optimization to reuse customized terminal parsers"""
		self.terminalParserCache[ (index,flags) ] = parser
	def getCustomTerminalParser( self, index, flags ):
		"""Retrieved a cached customized terminal parser or None"""
		return self.terminalParserCache.get( (index, flags))
		
	def getParserList (self):
		return self.parserList


	def getObjectForName( self, name):
		"""Determine whether our methodSource has a parsing method for the given name

		returns ( flags or 0 , tagobject)
		"""
		testName = "_m_"+name
		if hasattr( self.methodSource, testName):
			method = getattr( self.methodSource, testName )
			if callable(method):
				return  TextTools.CallTag, method
			elif method == TextTools.AppendMatch:
				return method, name
			elif method in (TextTools.AppendToTagobj, TextTools.AppendTagobj):
				object = self.getTagObjectForName( name )
				if method == TextTools.AppendToTagobj:
					if not ( hasattr( object, 'append') and callable(object.append)):
						raise ValueError( """Method source %s declares production %s to use AppendToTagobj method, but doesn't given an object with an append method in _o_%s (gave %s)"""%(repr(self.methodSource), name,name, repr(object)))
				return method, object
			else:
				raise ValueError( """Unrecognised command value %s (not callable, not one of the Append* constants) found in methodSource %s, name=%s"""%( repr(method),repr(methodSource),name))
		return 0, name
	def getTagObjectForName( self, name ):
		"""Get any explicitly defined tag object for the given name"""
		testName = "_o_"+name
		if hasattr( self.methodSource, testName):
			object = getattr( self.methodSource, testName )
			return object
		return name
	def addDefinitionSource( self, item ):
		"""Add a source for definitions when the current grammar doesn't supply
		a particular rule (effectively common/shared items for the grammar)."""
		self.definitionSources.append( item )


### Compatability API
##  This API exists to allow much of the code written with SimpleParse 1.0
##  to work with SimpleParse 2.0
class GeneratorAPI1:
	"""Stand-in class supporting operation of SimpleParse 1.0 applications

	There was really only the one method of interest, parserbyname,
	everything else was internal (and is now part of
	simpleparsegrammar.py).
	"""
	def __init__( self, production, prebuilt=() ):
		from simpleparse.parser import Parser
		self.parser = Parser( production, prebuilts=prebuilt )
	def parserbyname( self, name ):
		"""Retrieve a tag-table by production name"""
		return self.parser.buildTagger( name )

def buildParser( declaration, prebuiltnodes=() ):
	"""API 1.0 primary entry point, returns a GeneratorAPI1 instance

	That object will respond to the parserbyname API expected by
	SimpleParse 1.0 applications.
	"""
	return GeneratorAPI1( declaration, prebuiltnodes )

