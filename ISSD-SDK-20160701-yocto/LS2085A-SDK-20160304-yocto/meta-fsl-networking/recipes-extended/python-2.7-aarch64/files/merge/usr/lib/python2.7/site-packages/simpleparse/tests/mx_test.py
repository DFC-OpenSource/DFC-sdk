import mx_low, mx_flag, mx_high, mx_special, mx_recursive
import unittest


def getSuite():
	set = []
	for module in [
		mx_low,
		mx_flag,
		mx_high,
		mx_special,
		mx_recursive
	]:
		set.append( module.getSuite() )
	return unittest.TestSuite(
		set
	)

if __name__ == "__main__":
	unittest.main(defaultTest="getSuite")
