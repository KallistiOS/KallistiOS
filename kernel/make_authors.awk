# KallistiOS ##version##
#
# arch/dreamcast/kernel/make_authors.awk
# Copyright (C) 2013, 2014 Lawrence Sebald

# This script processes the AUTHORS file in the root directory of the source
# distribution to create the authors.h file that is needed in banner.c. This
# gives a relatively easy way to give credit to the people who have made KOS
# what it is over the years in your programs by simply using the compiled-in
# string with copyright information.
#
# Since this script relies on the format of the file as it stands now, if the
# formatting of the AUTHORS list changes, this will need to be updated as well.
BEGIN {
    FS = ":";
    phase = 0;
}
{
    if($0 == "Contributors list (under the normal KOS license):") {
        phase = 1;
    }
    else if($0 == "-------------------------------------------------" && phase == 1) {
        phase = 2;
        print "static const char authors[] = ";
    }
    else if($0 == "Files with Specific licenses:") {
        print ";\n";
        exit;
    }
    else if(phase == 2 && $0 != "") {
        gsub(/"/, "\\\"", $1)
        print "\"Copyright (C)" $2, $1 "\\n\"";
    }
}
