<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exsl="http://exslt.org/common" version="1.0" exclude-result-prefixes="exsl">
	<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl"/>

	<!-- add SF logo -->
	<xsl:template name="article.titlepage">
		<div class="titlepage">
		<table width="100%" cellspacing="0" cellpadding="0">
		<tr>
		<td width="100%">
			<xsl:variable name="recto.content">
				<xsl:call-template name="article.titlepage.before.recto"/>
				<xsl:call-template name="article.titlepage.recto"/>
			</xsl:variable>
			<xsl:variable name="recto.elements.count">
				<xsl:choose>
				        <xsl:when test="function-available('exsl:node-set')">
				        	<xsl:value-of select="count(exsl:node-set($recto.content)/*)"/>
				        </xsl:when>
					<xsl:otherwise>1</xsl:otherwise>
				</xsl:choose>
			</xsl:variable>
			<xsl:if test="(normalize-space($recto.content) != '') or ($recto.elements.count &gt; 0)">
				<div><xsl:copy-of select="$recto.content"/></div>
			</xsl:if>
			<xsl:variable name="verso.content">
				<xsl:call-template name="article.titlepage.before.verso"/>
				<xsl:call-template name="article.titlepage.verso"/>
			</xsl:variable>
			<xsl:variable name="verso.elements.count">
				<xsl:choose>
					<xsl:when test="function-available('exsl:node-set')">
						<xsl:value-of select="count(exsl:node-set($verso.content)/*)"/>
					</xsl:when>
					<xsl:otherwise>1</xsl:otherwise>
				</xsl:choose>
			</xsl:variable>
			<xsl:if test="(normalize-space($verso.content) != '') or ($verso.elements.count &gt; 0)">
				<div><xsl:copy-of select="$verso.content"/></div>
			</xsl:if>
		</td>
		<td valign="center">
			<a href="http://sourceforge.net">
				<img src="http://sflogo.sourceforge.net/sflogo.php?group_id=154582&amp;type=1" width="88" height="31" border="0" alt="SourceForge.net Logo" />
			</a>
		</td>
		</tr>
		</table>
			<xsl:call-template name="article.titlepage.separator"/>
		</div>
	</xsl:template>

</xsl:stylesheet>
