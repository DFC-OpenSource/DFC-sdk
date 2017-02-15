# Copyright (C) 2012 Linux Foundation
# Author: Richard Purdie
# Some code and influence taken from srctree.bbclass:
# Copyright (C) 2009 Chris Larson <clarson@kergoth.com>
# Released under the MIT license (see COPYING.MIT for the terms)
#
# externalsrc.bbclass enables use of an existing source tree, usually external to 
# the build system to build a piece of software rather than the usual fetch/unpack/patch
# process.
#
# To use, add externalsrc to the global inherit and set EXTERNALSRC to point at the
# directory you want to use containing the sources e.g. from local.conf for a recipe
# called "myrecipe" you would do:
#
# INHERIT += "externalsrc"
# EXTERNALSRC_pn-myrecipe = "/path/to/my/source/tree"
#
# In order to make this class work for both target and native versions (or with
# multilibs/cross or other BBCLASSEXTEND variants), B is set to point to a separate
# directory under the work directory (split source and build directories). This is
# the default, but the build directory can be set to the source directory if
# circumstances dictate by setting EXTERNALSRC_BUILD to the same value, e.g.:
#
# EXTERNALSRC_BUILD_pn-myrecipe = "/path/to/my/source/tree"
#

SRCTREECOVEREDTASKS ?= "do_patch do_unpack do_fetch"

python () {
    externalsrc = d.getVar('EXTERNALSRC', True)
    if externalsrc:
        d.setVar('S', externalsrc)
        externalsrcbuild = d.getVar('EXTERNALSRC_BUILD', True)
        if externalsrcbuild:
            d.setVar('B', externalsrcbuild)
        else:
            d.setVar('B', '${WORKDIR}/${BPN}-${PV}/')

        srcuri = (d.getVar('SRC_URI', True) or '').split()
        local_srcuri = []
        for uri in srcuri:
            if uri.startswith('file://'):
                local_srcuri.append(uri)
        d.setVar('SRC_URI', ' '.join(local_srcuri))

        if '{SRCPV}' in d.getVar('PV', False):
            # Dummy value because the default function can't be called with blank SRC_URI
            d.setVar('SRCPV', '999')

        tasks = filter(lambda k: d.getVarFlag(k, "task"), d.keys())

        for task in tasks:
            if task.endswith("_setscene"):
                # sstate is never going to work for external source trees, disable it
                bb.build.deltask(task, d)
            else:
                # Since configure will likely touch ${S}, ensure only we lock so one task has access at a time
                d.appendVarFlag(task, "lockfiles", "${S}/singletask.lock")

            # We do not want our source to be wiped out, ever (kernel.bbclass does this for do_clean)
            cleandirs = d.getVarFlag(task, 'cleandirs', False)
            if cleandirs:
                cleandirs = cleandirs.split()
                setvalue = False
                if '${S}' in cleandirs:
                    cleandirs.remove('${S}')
                    setvalue = True
                if externalsrcbuild == externalsrc and '${B}' in cleandirs:
                    cleandirs.remove('${B}')
                    setvalue = True
                if setvalue:
                    d.setVarFlag(task, 'cleandirs', ' '.join(cleandirs))

        fetch_tasks = ['do_fetch', 'do_unpack']
        # If we deltask do_patch, there's no dependency to ensure do_unpack gets run, so add one
        d.appendVarFlag('do_configure', 'deps', ['do_unpack'])

        for task in d.getVar("SRCTREECOVEREDTASKS", True).split():
            if local_srcuri and task in fetch_tasks:
                continue
            bb.build.deltask(task, d)

        d.prependVarFlag('do_compile', 'prefuncs', "externalsrc_compile_prefunc ")

        # Ensure compilation happens every time
        d.setVarFlag('do_compile', 'nostamp', '1')
}

python externalsrc_compile_prefunc() {
    # Make it obvious that this is happening, since forgetting about it could lead to much confusion
    bb.warn('Compiling %s from external source %s' % (d.getVar('PN', True), d.getVar('EXTERNALSRC', True)))
}
