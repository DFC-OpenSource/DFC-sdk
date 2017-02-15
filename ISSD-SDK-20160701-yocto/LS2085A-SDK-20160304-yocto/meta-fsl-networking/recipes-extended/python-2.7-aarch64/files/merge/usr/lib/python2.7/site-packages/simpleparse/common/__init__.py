"""Common (library) definitions

You normally use this module by importing one of our
sub-modules (which automatically registers itself with
the SOURCES list defined here).

Calling common.share( dictionary ) with a dictionary
mapping string names to element token instances will
make the element tokens available under those string
names in default parsers.  Note: a Parser can override
this by specifying an explicit definitionSources
parameter in its initialiser.
"""

def share( dictionary ):
	SOURCES.append( dictionary)

SOURCES = [
]