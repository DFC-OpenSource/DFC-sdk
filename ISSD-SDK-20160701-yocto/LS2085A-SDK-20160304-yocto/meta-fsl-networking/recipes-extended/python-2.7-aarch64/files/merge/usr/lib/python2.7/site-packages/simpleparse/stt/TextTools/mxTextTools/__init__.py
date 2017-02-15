""" mxTextTools - A tools package for fast text processing.

    Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
    Copyright (c) 2000-2003, eGenix.com Software GmbH; mailto:info@egenix.com
    Copyright (c) 2003-2006, Mike Fletcher; mailto:mcfletch@vrplumber.com
    See the documentation for further information on copyrights,
    or contact the author. All Rights Reserved.
"""
from simpleparse.stt.TextTools.mxTextTools.mxTextTools import *
from simpleparse.stt.TextTools.mxTextTools.mxTextTools import __version__

# To maintain backward compatibility:
BMS = TextSearch
BMSType = TextSearchType
try:
    TextSearch('',None,FASTSEARCH)
except:
    FS = BMS
    FSType = BMS
else:
    def FS(match, translate=None):
        return TextSearch(match, translate, FASTSEARCH)
    FSType = TextSearchType
