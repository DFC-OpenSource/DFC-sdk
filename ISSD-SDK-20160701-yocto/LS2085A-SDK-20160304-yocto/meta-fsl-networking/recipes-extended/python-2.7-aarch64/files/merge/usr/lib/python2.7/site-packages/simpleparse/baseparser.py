"""Base class for real-world parsers (such as parser.Parser)"""
from simpleparse.stt.TextTools.TextTools import *
from simpleparse.generator import Generator

class BaseParser:
	"""Class on which real-world parsers build

	Normally you use a sub-class of this class, such as
	simpleparser.parser.Parser
	"""
	_rootProduction = ""
	# primary API...
	def parse( self, data, production=None, processor=None, start=0, stop=None):
		"""Parse data with production "production" of this parser

		data -- data to be parsed, a Python string, for now
		production -- optional string specifying a non-default production to use
			for parsing data
		processor -- optional pointer to a Processor or MethodSource object for
			use in determining reporting format and/or post-processing the results
			of the parsing pass.  Can be None if neither is desired (default)
		start -- starting index for the parsing, default 0
		stop -- stoping index for the parsing, default len(data)
		"""
		self.resetBeforeParse()
		if processor is None:
			processor = self.buildProcessor()
		if stop is None:
			stop = len(data)
		value = tag( data, self.buildTagger( production, processor), start, stop )
		if processor and callable(processor):
			return processor( value, data )
		else:
			return value
	# abstract methods
	def buildProcessor( self ):
		"""Build default processor object for this parser class

		The default implementation returns None.  The processor
		can either implement the "method source" API (just provides
		information about Callouts and the like), or the processor
		API and the method-source API.  The processor API merely
		requires that the object be callable, and have the signature:
		
			object( (success, children, nextPosition), buffer)

		(Note: your object can treat the first item as a single tuple
		if it likes).

		See: simpleparse.processor module for details.
		"""
		return None
	def buildTagger( self, name, processor ):
		"""Build the tag-table for the parser

		This method must be implemented by your base class and _not_
		call the implementation here.
		"""
		raise NotImplementedError( """Parser sub-class %s hasn't implemented a buildTagger method"""%(self.__class__.__name__))
	def resetBeforeParse( self ):
		"""Called just before the parser's parse method starts working,

		Allows you to set up special-purpose structures, such as stacks
		or local storage values.  There is no base implementation.  The
		base implementation does nothing.
		"""
