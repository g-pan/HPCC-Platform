<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:output method="html"/>

    <xsl:template match="/Example">
        <xsl:variable name="examp1"><xsl:value-of select="SomeValue"/></xsl:variable>
        <html>
            <head>
                <title><xsl:value-of select="$attribute"/></title>
            </head>
            <body>
                <h1><xsl:value-of select="$attribute"/></h1>
            </body>
        </html>
    </xsl:template>
    
</xsl:stylesheet>
