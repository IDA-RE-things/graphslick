"""
BB matcher initialization script.

This script ensures that it runs once per Python runtime.


11/07/2013 - eliasb - Initial version
"""

import os
import sys
import idaapi

# Get the script path
lib_path = os.path.join(idaapi.idadir("plugins"), "GraphSlick")

# Run this code once by checking if the required scripts
# are in the path
if lib_path not in sys.path:
    sys.path.append(lib_path)
    print sys.path

    # Import the matcher module
    import bb_match

    #print "Imported"
else:
    #print "Already imported"
    pass
