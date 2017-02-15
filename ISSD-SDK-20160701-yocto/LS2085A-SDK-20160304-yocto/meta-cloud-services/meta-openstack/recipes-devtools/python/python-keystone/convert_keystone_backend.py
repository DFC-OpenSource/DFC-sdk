#!/usr/bin/python

import sys
import ConfigParser 
import shutil

path = "/etc/keystone/keystone.conf"

if len(sys.argv) != 2:
	sys.stderr.write("Usage: "+sys.argv[0]+" [sql|hybrid]\n")
	sys.exit(1)

backend = sys.argv[1]
if backend == "hybrid":
	identity_backend = 'keystone.identity.backends.hybrid_identity.Identity'
	assignment_backend = 'keystone.assignment.backends.hybrid_assignment.Assignment'
elif backend == "sql":
	identity_backend = 'keystone.identity.backends.sql.Identity'
	assignment_backend = 'keystone.assignment.backends.sql.Assignment'
else:
	sys.stderr.write("Usage: "+sys.argv[0]+" [sql|hybrid]\n")
	sys.exit(1)

shutil.copyfile(path, path + ".bak")

cfg = ConfigParser.ConfigParser()
c = cfg.read(path)

if not cfg.has_section("identity"):
	cfg.add_section("identity")

cfg.set("identity", "driver", identity_backend)

if not cfg.has_section("assignment"):
	cfg.add_section("assignment")

cfg.set("assignment", "driver", assignment_backend)

fp = open(path, "w")
cfg.write(fp)
fp.close()

exit(0)
