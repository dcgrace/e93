<!-- +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		File name: PseudoXPath.xsl

		Convert an XML file to PseudoXPath HTML elements
		
		Author: Michael Manning
		Version: 1.0

     +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:ssp="http://bw.ubs.com/ssp" xmlns:map="uri//bw.ubs.com/map" xmlns:java="http://xml.apache.org/xslt/java" exclude-result-prefixes="ssp map java" version="1.0">

    <xsl:output omit-xml-declaration="yes" method="xml" encoding="ISO-8859-1" indent="yes"/>

    <xsl:template match="*">
		<xsl:variable name="label">Element.<xsl:for-each select="ancestor::*"><xsl:value-of select="local-name()"/>#<xsl:value-of select="generate-id()"/>/</xsl:for-each><xsl:value-of select="local-name()"/>#<xsl:value-of select="generate-id()"/></xsl:variable>
        <xsl:element name="Input">
	 		<xsl:attribute name="type">hidden</xsl:attribute>
            <xsl:attribute name="name">
                <xsl:value-of select="$label"/>
            </xsl:attribute>
	        <xsl:attribute name="value">
            	<xsl:value-of select="normalize-space(text())"/>
	        </xsl:attribute>
        </xsl:element>
        <xsl:for-each select="@*">
	        <xsl:element name="Input">
	 			<xsl:attribute name="type">hidden</xsl:attribute>
	            <xsl:attribute name="name">
	                <xsl:value-of select="$label"/>@<xsl:value-of select="name()"/>
	            </xsl:attribute>
	            <xsl:attribute name="value">
	            	<xsl:value-of select="."/>
	            </xsl:attribute>
	        </xsl:element>
        </xsl:for-each>
        <xsl:apply-templates select="*"/>
    </xsl:template>
</xsl:stylesheet>
