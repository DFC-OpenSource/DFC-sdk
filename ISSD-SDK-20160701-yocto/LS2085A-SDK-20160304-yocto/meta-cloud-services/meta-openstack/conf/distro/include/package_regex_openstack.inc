# package_regex_openstack.inc - This file contains data that tracks
# upstream project associated with a given recipe. This list is needed
# for recipes that version information can not be automagically discovered.
# As we automate this checking, this file will slowly be reduced.
#
# This data is used by the auto-upgrade-helper, to use it, you could
# add the following to your conf/local.conf:
#   --------------- snip ---------------
#   INHERIT =+ "distrodata"
#   require conf/distro/include/package_regex_openstack.inc
#   --------------- snip ---------------
#
# Fore more details about auto-upgrade-help, please refer to:
#   http://git.yoctoproject.org/cgit/cgit.cgi/auto-upgrade-helper/tree/README
#
# The format is as a bitbake variable override for each recipe
#
#       REGEX_URI_pn-<recipe name> = "recipe_url"
#	 - This is the url used by the package checking system to
#	   get the latest version of the package
#       REGEX_pn-<recipe name> = "package_regex"
#	 - This is the regex the package checking system uses to
#	   parse the page found at REGEX_URI_pn-<recipe name>
#

COMMON_REGEX = "(?P<pver>(\d+[\.-_]*)+)"

# PyPI sites
REGEX_URI_pn-python-cephclient = "https://pypi.python.org/pypi/python-cephclient/"
REGEX_pn-python-cephclient = "/python-cephclient/${COMMON_REGEX}"
REGEX_URI_pn-python-hp3parclient = "https://pypi.python.org/pypi/hp3parclient/"
REGEX_pn-python-hp3parclient = "/hp3parclient/${COMMON_REGEX}"
REGEX_URI_pn-python-openstackclient = "https://pypi.python.org/pypi/python-openstackclient/"
REGEX_pn-python-openstackclient = "/python-openstackclient/${COMMON_REGEX}"

# GIT
GITTAGREGEX_COMMON = "(?P<pver>\d+\.\d+\.(\d\.?)*)"

GITTAGREGEX_pn-python-glanceclient = "${GITTAGREGEX_COMMON}"
GITTAGREGEX_pn-python-keystoneclient = "${GITTAGREGEX_COMMON}"
GITTAGREGEX_pn-python-neutronclient = "(?P<pver>\d+\.\d+\.(\d\.?)+)"
GITTAGREGEX_pn-python-novaclient = "${GITTAGREGEX_COMMON}"

GITTAGREGEX_BETA = "(?P<pver>\d+\.\d+(\.\d+(b\d)?)?)"
GITTAGREGEX_pn-python-barbican = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-ceilometer = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-cinder = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-glance = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-heat = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-horizon = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-keystone = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-neutron = "${GITTAGREGEX_BETA}"
GITTAGREGEX_pn-python-nova = "${GITTAGREGEX_BETA}"
