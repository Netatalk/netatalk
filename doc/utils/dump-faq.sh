#!/bin/sh
# ($Id: dump-faq.sh,v 1.3 2004-08-21 15:07:59 tkaiser Exp $)
# 
# quick&dirty script to extract the contents of the Netatalk FAQ PhpWiki. Set 
# $BaseURL to the index page. The script will output the stuff to stdout

BaseURL="http://netatalk.sourceforge.net/wiki/index.php/FrequentlyAskedQuestions"
URLFilter="http://netatalk.sourceforge.net/wiki/index.php/"

ExtractFAQ=false
MyCounter=1
MyTOC="$(mktemp /tmp/Netatalk-FAQ.XXXXXX || exit 1)"
MyContents="$(mktemp /tmp/Netatalk-FAQ.XXXXXX || exit 1)"

echo -e "Extracting PhpWiki contents\c" >&2
# Check lynx version
lynx -tagsoup -nolist -dump "${BaseURL}" >/dev/null
if [ $? -ne 0 ]; then
	echo -e "\nYour lynx binary \"$(which lynx)\" doesn't support the relevant parameters (-tagsoup -nolist -dump). Try to get a more recent version. Exiting." >&2
	exit 1
fi

# Start pulling out the contents from the FAQ Wiki

lynx -width=200 -tagsoup -dump "${BaseURL}" | grep "${URLFilter}" | grep -v "\?" | while read -r URL ; do
	MyFAQ="$(echo "${URL}" | cut -b5-)"
	if [ "${ExtractFAQ}" = "true" ]; then
		echo -e ".\c" >&2
		MyTempFile="$(mktemp /tmp/Netatalk-FAQ.XXXXX)"
		lynx -width=400 -tagsoup -nolist -dump "${MyFAQ}" >"${MyTempFile}"
		FirstLine=$(egrep -n  "Q: " "${MyTempFile}" | cut -f1 -d:)
		MyTOCEntry="Q${MyCounter}:$(sed -n "${FirstLine}p" "${MyTempFile}" | awk -F"Q:" '{print $2}')"
		echo -e "${MyTOCEntry}\n" | fold -s -w80 >>"${MyTOC}"

		lynx -width=80 -tagsoup -nolist -dump "${MyFAQ}" >"${MyTempFile}"
		FirstLine=$(egrep -n  "Q: " "${MyTempFile}" | cut -f1 -d:)
		MyTOCEntry="Q${MyCounter}:$(sed -n "${FirstLine}p" "${MyTempFile}" | awk -F"Q:" '{print $2}')"
		FirstLine=$(expr ${FirstLine} + 1)
		LastLine=$(egrep -n  "^   Back to the FAQ" "${MyTempFile}" | cut -f1 -d:)
		LastLine=$(expr ${LastLine} - 1)
		echo -e "--------------------------------------------------------------------------------\n\n${MyTOCEntry}" >>"${MyContents}"
		sed -n "${FirstLine},${LastLine}p" "${MyTempFile}" >>"${MyContents}"
		rm "${MyTempFile}"
		MyCounter=$(expr ${MyCounter} + 1)
	else
		echo -e ".\c" >&2
	fi

	if [ "${MyFAQ}" = "http://netatalk.sourceforge.net/wiki/index.php/RecentChanges" ]; then
		ExtractFAQ=true
	fi
done

echo " finished" >&2
echo -e "Netatalk FAQ -- dumped from the online version on $(date -u)\n"
echo -e "For the most recent version of the FAQ please visit\n${BaseURL}\n\n--------------------------------------------------------------------------------\n"
cat "${MyTOC}" "${MyContents}"
rm "${MyTOC}" "${MyContents}"
