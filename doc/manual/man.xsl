<?xml version='1.0'?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 
<xsl:import href="PATH_TO_XSL_STYLESHEETS_DIR/manpages/docbook.xsl"/> 

<!-- * Collect date from <refmiscinfo class="date"> -->
<xsl:param name="refentry.date.profile.enabled">1</xsl:param>
<xsl:param name="refentry.date.profile">
  (//refmiscinfo[@class='date'])[last()]
</xsl:param>

<!-- * Suppress extra :VERSION: -->
<xsl:param name="refentry.version.suppress">1</xsl:param>

<!-- * Example without numbering -->
<xsl:param name="local.l10n.xml" select="document('')"/>
<l:i18n xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0">
<l:l10n language="en">
 <l:context name="title">
    <l:template name="example" text="Example.&#160;%t"/>
 </l:context>
 <l:context name="title-numbered">
    <l:template name="example" text="Example.&#160;%t"/>
 </l:context>
</l:l10n>
</l:i18n>
</xsl:stylesheet>
