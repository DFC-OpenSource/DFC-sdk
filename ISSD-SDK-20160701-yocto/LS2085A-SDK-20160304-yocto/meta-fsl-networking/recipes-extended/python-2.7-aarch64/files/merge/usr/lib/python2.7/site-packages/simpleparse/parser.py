"""Real-world parsers using the SimpleParse EBNF"""
from simpleparse import baseparser, simpleparsegrammar, common

class Parser( baseparser.BaseParser ):
	"""EBNF-generated Parsers with results-handling

	The Parser is a two-stage object:
		Passed an EBNF definition during initialisation,
		it compiles the definition into a tagging table
		(which in turn requires creating a tagging table
		for parsing the EBNF).

		You then call the parser's parse method to
		perform the actual parsing of your data, with the
		parser passing the results to your processor object
		and then back to you.
	"""
	def __init__(
		self, declaration, root='root',
		prebuilts=(), 
		definitionSources=common.SOURCES,
	):
		"""Initialise the parser, creating the tagging table for it

		declaration -- simpleparse ebnf declaration of the language being parsed
		root -- root production used for parsing if none explicitly specified
		prebuilts -- sequence of (name,value) tuples with prebuilt tables, values
			can be either objectgenerator EventToken sub-classes or TextTools
			tables
		definitionSources -- dictionaries of common constructs for use
			in building your grammar
		"""
		self._rootProduction = root
		self._declaration = declaration
		self._generator = simpleparsegrammar.Parser(
			declaration, prebuilts,
			definitionSources = definitionSources,
		).generator
	def buildTagger( self, production=None, processor=None):
		"""Get a particular parsing table for a particular production"""
		if production is None:
			production = self._rootProduction
		if processor is None:
			processor = self.buildProcessor()
		return self._generator.buildParser(
			production,
			methodSource=processor,
		)
	
