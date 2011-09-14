<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:str="http://exslt.org/strings"
    exclude-result-prefixes="xsl str">

    <xsl:param name="dialect" select="''"/>
    <xsl:output method="text" encoding="utf-8" />

    <xsl:variable name="dbtype">
        <xsl:value-of select="$dialect"/>
    </xsl:variable>

    <xsl:template match="/">
        <xsl:text>-- DBTYPE=</xsl:text>
        <xsl:value-of select="$dbtype"/>
        <xsl:text>

</xsl:text>
        <xsl:apply-templates select="/schema/table" mode="create-table" />
        <xsl:apply-templates select="/schema/table" mode="create-fk-constr" />
    </xsl:template>

    <xsl:template match="table" mode="create-table">
        <xsl:apply-templates select="." mode="before-create-table"/>
        <xsl:text>CREATE TABLE </xsl:text>
        <xsl:value-of select="@name" />
        <xsl:text> (
</xsl:text>
        <xsl:apply-templates select="column" mode="typed-column"/>
        <!--
        <xsl:apply-templates select="column" mode="fk"/>
        -->
        <xsl:text>    PRIMARY KEY (</xsl:text>
        <xsl:apply-templates select="column[primary-key]" mode="pk"/>
        <xsl:text>)
)</xsl:text>
        <xsl:apply-templates select="." mode="suffix" />
        <xsl:text>;

</xsl:text>
        <xsl:apply-templates select="." mode="seq" />
        <xsl:apply-templates select="." mode="after-create-table"/>
    </xsl:template>

    <xsl:template match="table" mode="create-fk-constr">
        <xsl:apply-templates select="column" mode="fk-constr"/>
    </xsl:template>

    <xsl:template match="table" mode="before-create-table" />

    <xsl:template match="table" mode="after-create-table">
        <xsl:if test="$dbtype = 'INTERBASE'">
            <xsl:text>COMMIT;

</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="table" mode="seq" />

    <xsl:template match="table[@sequence]" mode="seq">
        <xsl:choose>
            <xsl:when test="$dbtype = 'ORACLE'">
                <xsl:text>CREATE SEQUENCE </xsl:text>
                <xsl:value-of select="@sequence" />
                <xsl:text>;

</xsl:text>
            </xsl:when>
            <xsl:when test="$dbtype = 'INTERBASE'">
                <xsl:text>CREATE GENERATOR </xsl:text>
                <xsl:value-of select="@sequence" />
                <xsl:text>;

</xsl:text>
            </xsl:when>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="table" mode="suffix">
        <xsl:if test="$dbtype = 'MYSQL'">
            <xsl:text> ENGINE=INNODB DEFAULT CHARSET=utf8</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="column" mode="typed-column">
        <xsl:text>    </xsl:text>
        <xsl:value-of select="@name" />
        <xsl:text> </xsl:text>
        <xsl:apply-templates select="." mode="sql-type" />
        <xsl:apply-templates select="." mode="not-null-def" />
        <xsl:apply-templates select="." mode="autoinc" />
        <xsl:text>,
</xsl:text>
    </xsl:template>

    <xsl:template match="column" mode="not-null-def">
        <xsl:choose>
            <xsl:when test="$dbtype = 'MYSQL'">
                <xsl:apply-templates select="." mode="not-null" />
                <xsl:apply-templates select="." mode="def-value" />
            </xsl:when>
            <xsl:otherwise>
                <xsl:apply-templates select="." mode="def-value" />
                <xsl:apply-templates select="." mode="not-null" />
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="column" mode="fk" />

    <xsl:template match="column[foreign-key]" mode="fk">
        <xsl:text>    </xsl:text>
        <xsl:apply-templates select="." mode="fk-rule" />
        <xsl:text>,
</xsl:text>
    </xsl:template>

    <xsl:template match="column[foreign-key]" mode="fk-rule">
        <xsl:text>FOREIGN KEY (</xsl:text>
        <xsl:value-of select="@name" />
        <xsl:text>) REFERENCES </xsl:text>
        <xsl:value-of select="foreign-key/@table" />
        <xsl:text>(</xsl:text>
        <xsl:choose>
            <xsl:when test="foreign-key/@key">
                <xsl:value-of select="foreign-key/@key" />
            </xsl:when>
            <xsl:otherwise>
                <xsl:variable name="fk" select="foreign-key/@table"/>
                <xsl:value-of select="../../table[@name = $fk]
                    /column[primary-key]/@name"/>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:text>)</xsl:text>
    </xsl:template>

    <xsl:template match="column" mode="fk-constr" />

    <xsl:template match="column[foreign-key]" mode="fk-constr">
        <xsl:text>ALTER TABLE </xsl:text>
        <xsl:value-of select="../@name" />
        <xsl:text> ADD </xsl:text>
        <xsl:apply-templates select="." mode="fk-rule" />
        <xsl:text>;
</xsl:text>
    </xsl:template>

    <xsl:template match="column" mode="pk" />

    <xsl:template match="column[primary-key]" mode="pk">
        <xsl:if test="position() != 1">
            <xsl:text>, </xsl:text>
        </xsl:if>
        <xsl:value-of select="@name" />
    </xsl:template>

    <xsl:template match="column" mode="not-null" />

    <xsl:template match="column[primary-key]" mode="not-null">
        <xsl:text> NOT NULL</xsl:text>
    </xsl:template>

    <xsl:template match="column[@null and @null = 'false']"
            mode="not-null">
        <xsl:text> NOT NULL</xsl:text>
    </xsl:template>

    <xsl:template match="column" mode="def-value" />

    <xsl:template match="column[@default]" mode="def-value">
        <xsl:text> DEFAULT </xsl:text>
        <xsl:apply-templates select="." mode="typed-def-value" />
    </xsl:template>

    <xsl:template match="column" mode="typed-def-value">
        <xsl:value-of select="@default"/>
    </xsl:template>

    <xsl:template match="column[@type = 'string' or (@type = 'datetime'
                and @default != 'sysdate')]"
            mode="typed-def-value">
        <xsl:text>'</xsl:text>
        <xsl:value-of select="@default"/>
        <xsl:text>'</xsl:text>
    </xsl:template>

    <xsl:template match="column[@type = 'datetime' and @default = 'sysdate']"
            mode="typed-def-value">
        <xsl:choose>
            <xsl:when test="$dbtype = 'ORACLE'">
                <xsl:text>SYSDATE</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>CURRENT_TIMESTAMP</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="column" mode="autoinc" />

    <xsl:template match="column[primary-key and
            (../@sequence or ../@autoinc)]" mode="autoinc">
        <xsl:if test="$dbtype = 'MYSQL'">
            <xsl:text> AUTO_INCREMENT</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="column[@type = 'decimal']" mode="sql-type">
        <xsl:choose>
            <xsl:when test="$dbtype = 'ORACLE'">
                <xsl:text>NUMBER</xsl:text>
            </xsl:when>
            <xsl:when test="$dbtype = 'MYSQL' or $dbtype = 'INTERBASE'">
                <xsl:text>DECIMAL(16, 6)</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>DECIMAL</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="column[@type = 'datetime']" mode="sql-type">
        <xsl:choose>
            <xsl:when test="$dbtype = 'ORACLE'">
                <xsl:text>DATE</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>TIMESTAMP</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="column[@type = 'longint']" mode="sql-type">
        <xsl:choose>
            <xsl:when test="$dbtype = 'ORACLE'">
                <xsl:text>NUMBER</xsl:text>
            </xsl:when>
            <xsl:when test="$dbtype = 'POSTGRES'">
                <xsl:text>INT8</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>BIGINT</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="column[@type = 'string']" mode="sql-type">
        <xsl:choose>
            <xsl:when test="$dbtype = 'ORACLE'">
                <xsl:text>VARCHAR2</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>VARCHAR</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:text>(</xsl:text>
        <xsl:value-of select="@size" />
        <xsl:text>)</xsl:text>
    </xsl:template>

</xsl:stylesheet>
<!-- vim:ts=4:sts=4:sw=4:et:
-->
