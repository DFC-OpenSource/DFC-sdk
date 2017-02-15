"""Definition of the ParserSyntaxError raised on parse failure"""
import string
from simpleparse.stt.TextTools.TextTools import countlines

class ParserSyntaxError( SyntaxError ):
	"""Sub-class of SyntaxError for use by SimpleParse parsers

	Every instance will have the following attributes:
		buffer -- pointer to the source buffer
		position -- integer position in buffer where error occured or -1
		production -- the production which failed
		expected -- string (currently taken from grammar) describing
			what production/element token failed to match
	the following will be calculated in order to display
	human-friendly error messages:
		line -- ~ text line-number or -1
		lineChar -- ~ character on line where parsing failed or -1
	
	"""
	buffer = ""
	position = -1
	line = -1
	production = ""
	expected = ""
	DEFAULTTEMPLATE = """Failed parsing production "%(production)s" @pos %(position)s (~line %(line)s:%(lineChar)s).\nExpected syntax: %(expected)s\nGot text: %(text)s"""
	def __str__( self ):
		"""Create a string representation of the error"""
		if self.message:
			return '%s: %s'%( self.__class__.__name__, self.messageFormat(self.message) )
		else:
			return '%s: %s'%( self.__class__.__name__, self.messageFormat() )
	def messageFormat( self, template=None):
		"""Create a default message for this syntax error"""
		if template is None:
			template = self.DEFAULTTEMPLATE
		line, lineChar = self.getLineCoordinate()
		variables = {
			"production": self.production,
			"position": self.position,
			"line": line,
			"lineChar": lineChar,
			"expected": self.expected or "UNKNOWN",
			"text": repr(self.buffer[ self.position:self.position+50 ]),
		}
		return template % variables
	def getLineCoordinate( self ):
		"""Get (line number, line character) for the error"""
		lineChar = string.rfind( self.buffer, '\n', 0, self.position)
		if lineChar == -1: # was no \n before the current position
			lineChar = self.position
			line = 1
		else:
			line = countlines( self.buffer[:lineChar] )
			lineChar = self.position-lineChar
		return line, lineChar
