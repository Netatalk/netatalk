<?xml version='1.0'?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 
	<xsl:import href="PATH_TO_XSL_STYLESHEETS_DIR/xhtml/chunk.xsl"/> 
	<xsl:param name="use.id.as.filename" select="'1'"/>
	<xsl:param name="chunk.section.depth" select="0"></xsl:param>
	<xsl:param name="chunk.separate.lots" select="1"></xsl:param>
	<xsl:param name="html.stylesheet" select="'http://netatalk.sourceforge.net/css/netatalk.css'"/>

	<xsl:template name="user.header.navigation">
		<xsl:variable name="codefile" select="document('netatalk.html',/)"/>
		<xsl:copy-of select="$codefile/*/node()"/>
	</xsl:template>
</xsl:stylesheet>
