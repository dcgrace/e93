<?xml version="1.0" encoding="ISO-8859-1"?>

<!-- +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      File name: Chapters.xsl

      Author: Michael Manning
      Version: 2.0
      
      Transforms from XML "chapter" documentation to HTML

     +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ -->
     
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
   <xsl:output method="html"/>
   <xsl:output indent="yes"/>

<!-- ==============
      ROOT ELEMENT
     ============== -->

  <xsl:template match="/">
      <xsl:element name="HTML">
         <xsl:call-template name="MakeHTMLHeader"/>
      		<xsl:element name="CENTER">
	      		<xsl:element name="H1">
	         	<xsl:value-of select="normalize-space(/CHAPTER/TITLE)"/>
		      </xsl:element>
	      </xsl:element>
         <xsl:apply-templates select="CHAPTER/BODY"/>
      </xsl:element>
   </xsl:template>
   
<!-- ==============
      HTML HEADER
     ============== -->
   <xsl:template name="MakeHTMLHeader">
      <xsl:element name="HEAD">
         <xsl:element name="TITLE">
            <xsl:value-of select="normalize-space(/CHAPTER/TITLE)"/>
         </xsl:element>
         <xsl:element name="LINK">
            <xsl:attribute name="REL">STYLESHEET</xsl:attribute>
            <xsl:attribute name="HREF">cmdref.css</xsl:attribute>
            <xsl:attribute name="CHARSET">ISO-8859-1</xsl:attribute>
            <xsl:attribute name="TYPE">text/css</xsl:attribute>
         </xsl:element>
         <xsl:apply-templates select="/CHAPTER/SUMMARY"/>
      </xsl:element>
   </xsl:template>
   
<!-- ======
      Don't strip out the imbeded HTML, just copy the elements and their attributes
     ====== -->
   <xsl:template match="*">
	  <xsl:text> </xsl:text>
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   
<!-- ======
      SUMMARYS (don't appear as text)
     ====== -->
   <xsl:template match="SUMMARY">
   		<xsl:call-template name="MakeSummary"><xsl:with-param name="contents" select="normalize-space(text())"/></xsl:call-template>
   </xsl:template>

	<xsl:template name="MakeSummary">
	  <xsl:param name="contents"/>
      <xsl:comment>
         <xsl:text>
         &lt;OBJECT type="text/summary"&gt;</xsl:text><xsl:value-of select="$contents"/><xsl:text>&lt;/OBJECT&gt;
         </xsl:text>
      </xsl:comment>
	</xsl:template>

</xsl:stylesheet>
