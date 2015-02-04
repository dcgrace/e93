<?xml version="1.0" encoding="ISO-8859-1"?>

<!-- +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      File name: Clean.xsl

      Author: Michael Manning
      Version: 1.0

     +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ -->

<xsl:stylesheet
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xalan="http://xml.apache.org/xslt" 
	version="1.0">

	<xsl:output omit-xml-declaration="yes" method="xml" encoding="ISO-8859-1" indent="yes" xalan:indent-amount="4"/>

	<xsl:template match="@*|node()|*">
	    <xsl:copy>
	        <xsl:copy-of select="@*"/>
	        <xsl:apply-templates/>
	    </xsl:copy>
	</xsl:template>

<!-- ======
      drop TempRootWrapperForCleaning
     ====== -->
   <xsl:template match="TempRootWrapperForCleaning">
		<xsl:apply-templates/>
   </xsl:template>
   
<!-- ======
      Copy inline with space
     ====== -->
   <xsl:template match="Strong">
		<xsl:text> </xsl:text><xsl:copy>
			<xsl:copy-of select="@*"/>
			<xsl:apply-templates/>
		</xsl:copy><xsl:text> </xsl:text>
   </xsl:template>

<!-- ======
      Copy inline
     ====== -->
   <xsl:template match="xsl:value-of">
		<xsl:copy> <xsl:copy-of select="@*"/> <xsl:apply-templates/></xsl:copy>
   </xsl:template>

<!-- ======
      Copy comments
     ====== -->
   <xsl:template match="comment()">
   		<xsl:comment><xsl:value-of select="normalize-space(.)"/></xsl:comment>
   </xsl:template>

<!-- ======
      Copy text nodes
     ====== -->
   <xsl:template match="text()">
   		<xsl:value-of select="normalize-space(.)"/>
   </xsl:template>

</xsl:stylesheet>
