import sys
from PyQt4 import QtCore, QtGui, QtSvg
from xml.dom import minidom
import time
import math
import optparse
import os
import threading


class Grid:
    def __init__(self, fname):
        self.done = False
        print "loading grid from ",fname
        self.xmldoc = minidom.parse(fname)
        svg = self.xmldoc.getElementsByTagName('svg')
        cells=svg[0].getElementsByTagName('polygon')
        print "read ",len(cells), "cells"
        self.cells = {}
        for c in cells:
            cid = c.getAttribute("id")
            if 'id' not in cid:
                print "invalid cell id ",cidg
            self.cells[cid] = c
        self.current_layer_ix = 0
        
    def listen(self):
        while True:
            print "refreshing @ ", time.time()

            time.sleep(1)
            if self.done:
                return



if __name__ == "__main__":
    parser = optparse.OptionParser(usage="usage: %prog [options] <grid svg template>",
                                   version="%prog 1.0")
    parser.add_option("-k", "--keep-images",
                      action="store_true",
                      help="keep images")
    
    parser.add_option("-o", "--output-dir",
                      default="./",
                      help="where to keep images")
    
    parser.add_option("-f", "--svg-file",
                      default="./grid.svg",
                      help="temporary svg image")

    (options, args) = parser.parse_args()
    if len(args) != 1:
        print "mandatory arguments missings"
        parser.print_help()
        exit(-1)

    inigrid = args[0]
    svgfile = options.svg_file

    keep_images = options.keep_images
    output_dir = options.output_dir
    if keep_images:
        print "storing images in ",output_dir

    grid = Grid(inigrid)
    
    tid = threading.Thread(target=grid.listen)
    tid.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print "exiting ..."
        grid.done = True
        tid.join()
    print "good bye"

                
