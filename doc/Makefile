#
# Makefile for Netatalk Documentatation
# Requires GNU make and GNU sed
#
# Available targets:
#   html: make html doc
#   man: make manpages
#   install: copy manpages to Netatalk source code tree

# VPATH = manual/man/man1:manual/man/man3:manual/man/man4:manual/man/man5:manual/man/man8

# setup
sed=sed
OS=$(shell uname -s)

ifeq (SunOS, $(findstring SunOS, $(OS)))
	sed=gsed
endif

# Dependencies

SUFFIXES = .xml .

MAN_XSL  = man.xsl
HTML_XSL = html.xsl
TMPDIR   = tmp

MAN_XSL_TMP  = $(TMPDIR)/$(MAN_XSL)
HTML_XSL_TMP = $(TMPDIR)/$(HTML_XSL)

manpages = manpages/ad.1 \
		manpages/aecho.1 \
		manpages/afpldaptest.1 \
		manpages/afppasswd.1 \
		manpages/apple_dump.1 \
		manpages/asip-status.pl.1 \
		manpages/dbd.1 \
		manpages/getzones.1 \
		manpages/macusers.1 \
		manpages/megatron.1 \
		manpages/nbp.1 \
		manpages/netatalk-config.1 \
		manpages/pap.1 \
		manpages/psorder.1  \
		manpages/uniconv.1 \
		manpages/atalk_aton.3 \
		manpages/nbp_name.3 \
		manpages/atalk.4 \
		manpages/afpd.conf.5  \
		manpages/afp_signature.conf.5  \
		manpages/afp_ldap.conf.5 \
		manpages/afp_voluuid.conf.5 \
		manpages/papd.conf.5 \
		manpages/AppleVolumes.default.5 \
		manpages/atalkd.conf.5 \
		manpages/netatalk.conf.5 \
		manpages/afpd.8 \
		manpages/atalkd.8 \
		manpages/cnid_dbd.8 \
		manpages/cnid_metad.8 \
		manpages/papd.8 \
		manpages/papstatus.8 \
		manpages/psf.8 \
		manpages/timelord.8

# General targets

.PHONY:	all clean install

all:
		@echo Read the README

man:	manpageinit $(manpages)
html: 	htmlpages

tmpdir:
		@if [ ! -d $(TMPDIR) ] ; then \
			mkdir $(TMPDIR); \
		fi

clean:
		rm -f manpages/*
		rm -f html/*
		rm -f tmp/*
		rm -f html.tgz

# manpage targets

manpages/%.1 : manual/man/man1/%.1.xml
		@xsltproc -o manpages/ $(MAN_XSL_TMP) $<
		@$(sed) -i -e "s@:NETATALK_VERSION:@Netatalk $(VERSION)@g" $@

manpages/%.3 : manual/man/man3/%.3.xml
		@xsltproc -o manpages/ $(MAN_XSL_TMP) $<
		@$(sed) -i -e "s@:NETATALK_VERSION:@Netatalk $(VERSION)@g" $@

manpages/%.4 : manual/man/man4/%.4.xml
		@xsltproc -o manpages/ $(MAN_XSL_TMP) $<
		@$(sed) -i -e "s@:NETATALK_VERSION:@Netatalk $(VERSION)@g" $@

manpages/%.5 : manual/man/man5/%.5.xml
		@xsltproc -o manpages/ $(MAN_XSL_TMP) $<
		@$(sed) -i -e "s@:NETATALK_VERSION:@Netatalk $(VERSION)@g" $@

manpages/%.8 : manual/man/man8/%.8.xml
		@xsltproc -o manpages/ $(MAN_XSL_TMP) $<
		@$(sed) -i -e "s@:NETATALK_VERSION:@Netatalk $(VERSION)@g" $@

$(MAN_XSL_TMP) : manual/man.xsl
		@if [ "x$(XSL)" = "x" ] ; then \
			echo 'Set $$XSL to be the path of your XSL stylesheet directory.'; \
			exit 1; \
		fi
		@cp manual/man.xsl $(MAN_XSL_TMP)
		@echo Configuring XSL stylesheet with path $(XSL)
		@$(sed) -i -e "s@PATH_TO_XSL_STYLESHEETS_DIR@$(XSL)@" $(MAN_XSL_TMP)

manpageinit: tmpdir $(MAN_XSL_TMP)
		@if [ "x$(VERSION)" = "x" ] ; then \
			echo 'Set $$VERSION to the Netatalk version'; \
			exit 1; \
		fi
		@if [ ! -d manpages ] ; then \
			mkdir manpages; \
		fi

install:
		@if [ "x$(DIR)" = "x" ] ; then \
			echo 'Set $$DIR to be the path of your Netatalk directory.'; \
			exit 1; \
		fi
		sh checkinmans.sh "$(DIR)"

# html targets

html-upload: htmlpages
	@if [ "x$(USER)" = "x" ] ; then \
		echo 'Set $$USER to the Sourceforge user'; \
		exit 1; \
	fi
	scp html/* $(USER),netatalk@web.sourceforge.net:/home/project-web/netatalk/htdocs/2.2/htmldocs/

htmlpages:	htmlinit
		@xsltproc -o html/ $(HTML_XSL_TMP) manual/manual.xml
		@find html -name '*.html' -exec $(sed) -i -e "s@:SBINDIR:/@@g" -e "s@:BINDIR:/@@g" \
			-e "s@:ETCDIR:/@@g" -e "s@:LIBDIR:/@@g" -e "s@:LIBEXECDIR:/@@g" \
			-e "s@:DESTDIR:/@@g" \
			-e "s@:VERSION:@$(VERSION)@g" {} \;

htmlinit:	tmpdir $(HTML_XSL_TMP)
		@if [ "x$(VERSION)" = "x" ] ; then \
			echo 'Set $$VERSION to the Netatalk version'; \
			exit 1; \
		fi
		@if [ ! -d html ] ; then \
			mkdir html; \
		fi

$(HTML_XSL_TMP) : manual/html.xsl
		@if [ "x$(XSL)" = "x" ] ; then \
			echo 'Set $$XSL to be the path of your XSL stylesheet directory.'; \
			exit 1; \
		fi
		@cp manual/html.xsl $(HTML_XSL_TMP)
		@echo Configuring XSL stylesheet with path $(XSL)
		@$(sed) -i -e "s@PATH_TO_XSL_STYLESHEETS_DIR@$(XSL)@" $(HTML_XSL_TMP)

