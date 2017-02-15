do_install_append() {
    sed -i '/argparse/d' ${D}${libdir}/python2.7/site-packages/*/requires.txt
}
