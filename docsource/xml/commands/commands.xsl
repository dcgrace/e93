<?xml version="1.0" encoding="ISO-8859-1"?>

<!-- +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      File name: Commands.xsl

      Author: Michael Manning
      Version: 2.0
      
      Transforms from XML "command" documentation to HTML

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
         <xsl:element name="BODY">
            <TABLE WIDTH="100%" BORDER="0" CELLSPACING="0" CELLPADDING="0">
               <TR>
                  <TD WIDTH="60%" ALIGN="LEFT">
                     <H1>
                        <xsl:value-of select="normalize-space(COMMAND/NAME)"/>
                     </H1>
                  </TD>
                  <TD WIDTH="40%" ALIGN="RIGHT">
                     <H1>
                        <xsl:choose>
                           <xsl:when test="COMMAND/@type">
                              <xsl:value-of select="COMMAND/@type"/>
                           </xsl:when>
                           <xsl:otherwise>Built-in</xsl:otherwise>
                        </xsl:choose>
                     </H1>
                  </TD>
               </TR>
            </TABLE>
            <xsl:element name="H3">SYNOPSIS</xsl:element>
            <xsl:element name="BLOCKQUOTE">
               <xsl:element name="P">
               	<xsl:element name="CODE">
                     <xsl:value-of select="normalize-space(COMMAND/NAME)"/>
                     <xsl:apply-templates select="COMMAND/PARAMETER" mode="synopsis"/>
                     <xsl:apply-templates select="COMMAND/OPTION" mode="synopsis"/>
               	  </xsl:element>
               </xsl:element>
            </xsl:element>
            <xsl:apply-templates select="COMMAND/DESCRIPTION"/>
            <xsl:apply-templates select="COMMAND/OUTPUT"/>
            <xsl:element name="H3">PARAMETERS</xsl:element>
            <xsl:element name="BLOCKQUOTE">
               <xsl:choose>
                  <xsl:when test="COMMAND/PARAMETER!=''">
                     <xsl:apply-templates select="COMMAND/PARAMETER"/>
                  </xsl:when>
                  <xsl:otherwise>
                     <xsl:element name="P">NONE</xsl:element>
                  </xsl:otherwise>
               </xsl:choose>
            </xsl:element>
            <xsl:element name="H3">OPTIONS</xsl:element>
            <xsl:element name="BLOCKQUOTE">
               <xsl:choose>
                  <xsl:when test="COMMAND/OPTION!=''">
                     <xsl:apply-templates select="COMMAND/OPTION"/>
                  </xsl:when>
                  <xsl:otherwise>
                     <xsl:element name="P">NONE</xsl:element>
                  </xsl:otherwise>
               </xsl:choose>
            </xsl:element>
            <xsl:apply-templates select="COMMAND/STATUS"/>
            <xsl:apply-templates select="COMMAND/EXAMPLES"/>
            <xsl:apply-templates select="COMMAND/LIMITATIONS"/>
            <xsl:apply-templates select="COMMAND/LOCATION"/>
            <xsl:apply-templates select="COMMAND/SEEALSO"/>
         </xsl:element>
      </xsl:element>
   </xsl:template>
   
<!-- ==============
      HTML HEADER
     ============== -->
   <xsl:template name="MakeHTMLHeader">
      <xsl:element name="HEAD">
         <xsl:element name="TITLE">
            <xsl:value-of select="normalize-space(COMMAND/NAME)"/>
         </xsl:element>
         <xsl:element name="LINK">
            <xsl:attribute name="REL">STYLESHEET</xsl:attribute>
            <xsl:attribute name="HREF">../cmdref.css</xsl:attribute>
            <xsl:attribute name="CHARSET">ISO-8859-1</xsl:attribute>
            <xsl:attribute name="TYPE">text/css</xsl:attribute>
         </xsl:element>
         <xsl:apply-templates select="COMMAND/SUMMARY"/>
         <xsl:if test="not(COMMAND/SUMMARY)">
   			<xsl:call-template name="MakeSummary"><xsl:with-param name="contents"><xsl:value-of select="normalize-space(substring-before(COMMAND/DESCRIPTION,'.'))"/></xsl:with-param></xsl:call-template>
         </xsl:if>
      </xsl:element>
   </xsl:template>
   
<!-- ======
      COMMAND
     ====== -->
   <xsl:template match="COMMAND">
      <xsl:apply-templates/>
   </xsl:template>

<!-- ======
      NAME
     ====== -->
   <xsl:template match="NAME">
   </xsl:template>

<!-- ======
      PARAMETERS & OPTIONS
     ====== -->
   <xsl:template match="PARAMETER|OPTION">
      <xsl:element name="A">
         <xsl:attribute name="name">
            <xsl:value-of select="normalize-space(NAME)"/>
         </xsl:attribute>
      </xsl:element>
      <xsl:element name="P">
         <xsl:element name="VAR">
            <xsl:value-of select="normalize-space(NAME)"/>
         </xsl:element>
         <xsl:element name="BLOCKQUOTE">
            <xsl:element name="P">
                <xsl:apply-templates/>
   	           	<xsl:if test="count(CHOICE)=2">
            		<xsl:element name="P">
            		This may be either one of the following values:
            		</xsl:element>
            	</xsl:if>
            	<xsl:if test="count(CHOICE)>2">
            		<xsl:element name="P">
            		This may be any one of the following values:
            		</xsl:element>
            	</xsl:if>
               	<xsl:apply-templates select="CHOICE"/>
            </xsl:element>
         </xsl:element>
      </xsl:element>
   </xsl:template>
   
<!-- ======
      CHOICES
     ====== -->
   <xsl:template match="CHOICE">
      <xsl:element name="P">
         <xsl:element name="VAR">
            <xsl:value-of select="normalize-space(NAME)"/>
         </xsl:element>
         <xsl:element name="BLOCKQUOTE">
            <xsl:element name="P">
               <xsl:apply-templates/>
            </xsl:element>
         </xsl:element>
      </xsl:element>
   </xsl:template>
   
<!-- ======
      get the names of PARAMETERS & OPTIONS for synopses (and checks if they're optional)
     ====== -->
   <xsl:template match="PARAMETER|OPTION" mode="synopsis">
      <xsl:text> </xsl:text>
      <xsl:element name="A">
         <xsl:attribute name="href">#<xsl:value-of select="normalize-space(NAME)"/>
         </xsl:attribute>
         <xsl:element name="VAR">
            <xsl:choose>
               <xsl:when test="@optional">
					?<xsl:value-of select="normalize-space(NAME)"/>?
				</xsl:when>
               <xsl:otherwise>
                  <xsl:value-of select="normalize-space(NAME)"/>
               </xsl:otherwise>
            </xsl:choose>
         </xsl:element>
      </xsl:element>
   </xsl:template>
   
<!-- ======
      SECTIONS
     ====== -->
   <xsl:template match="DESCRIPTION|STATUS|OUTPUT|EXAMPLES|LIMITATIONS|LOCATION|SEEALSO">
      <xsl:element name="H3">
         <xsl:value-of select="name()"/>
      </xsl:element>
      <xsl:element name="BLOCKQUOTE">
         <xsl:element name="P">
            <xsl:apply-templates/>
         </xsl:element>
      </xsl:element>
   </xsl:template>
   
 <!-- ======
      Don't strip out the embedded HTML, just copy the elements and their attributes
     ====== -->
   <xsl:template match="*">
	   <xsl:text> </xsl:text>
      <xsl:copy>
         <xsl:copy-of select="@*"/>
         <xsl:apply-templates/>
      </xsl:copy>
   </xsl:template>
   
<!-- ======
      SUMMARYS (don't appear as text) Convert them to HTML comments with an embedded object
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
