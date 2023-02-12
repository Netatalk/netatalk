#!/bin/sh
# 
# This script requires asciidoc http://www.methods.co.nz/asciidoc/

# (echo -e "Netatalk NEWS\n-------------\n\n" ; cat NEWS) | ./asciidoc.py -f netatalk-news.conf -b css -o NEWS.html - && chmod g+w NEWS.html
./asciidoc.py -f netatalk-relnotes.conf -b html5 -o ReleaseNotes.html ReleaseNotes && chmod g+w ReleaseNotes.html
