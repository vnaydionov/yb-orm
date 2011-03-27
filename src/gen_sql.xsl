<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:str="http://exslt.org/strings"
    exclude-result-prefixes="xsl str">

    <xsl:output method="text" encoding="utf-8" />

    <xsl:template match="/">
        <xsl:apply-templates select="/scheme/table" mode="create-table" />
    </xsl:template>

    <xsl:template match="table" mode="create-table">
        <xsl:text>CREATE TABLE </xsl:text>
        <xsl:value-of select="@name" />
        <xsl:text> (
</xsl:text>
        <xsl:apply-templates select="columns/column" mode="typed-column"/>
        <xsl:apply-templates select="columns/column" mode="fk"/>
        <xsl:text>    PRIMARY KEY (</xsl:text>
        <xsl:apply-templates select="columns/column[primarykey]" mode="pk"/>
        <xsl:text>)
)</xsl:text>
        <xsl:apply-templates select="." mode="suffix" />
        <xsl:text>;

</xsl:text>
        <xsl:apply-templates select="." mode="seq" />
    </xsl:template>

    <xsl:template match="table" mode="seq" />

    <xsl:template match="table[/scheme[@dbtype = 'ORACLE'] and @sequence]"
            mode="seq">
        <xsl:text>CREATE SEQUENCE </xsl:text>
        <xsl:value-of select="@sequence" />
        <xsl:text>;

</xsl:text>
    </xsl:template>

    <xsl:template match="table" mode="suffix" />

    <xsl:template match="table[/scheme[@dbtype = 'MYSQL']]"
            mode="suffix">
        <xsl:text> ENGINE=INNODB DEFAULT CHARSET=utf8</xsl:text>
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
        <xsl:apply-templates select="." mode="def-value" />
        <xsl:apply-templates select="." mode="not-null" />
    </xsl:template>

    <xsl:template match="column[/scheme[@dbtype = 'MYSQL']]"
            mode="not-null-def">
        <xsl:apply-templates select="." mode="not-null" />
        <xsl:apply-templates select="." mode="def-value" />
    </xsl:template>

    <xsl:template match="column" mode="fk" />

    <xsl:template match="column[foreign-key]" mode="fk">
        <xsl:text>    FOREIGN KEY (</xsl:text>
        <xsl:value-of select="@name" />
        <xsl:text>) REFERENCES </xsl:text>
        <xsl:value-of select="foreign-key/@table" />
        <xsl:text>(</xsl:text>
        <xsl:value-of select="foreign-key/@field" />
        <xsl:text>),
</xsl:text>
    </xsl:template>

    <xsl:template match="column" mode="pk" />

    <xsl:template match="column[primarykey]" mode="pk">
        <xsl:if test="position() != 1">
            <xsl:text>, </xsl:text>
        </xsl:if>
        <xsl:value-of select="@name" />
    </xsl:template>

    <xsl:template match="column" mode="not-null" />

    <xsl:template match="column[primarykey]" mode="not-null">
        <xsl:text> NOT NULL</xsl:text>
    </xsl:template>

    <xsl:template match="column[@nullable and @nullable = '0']"
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

    <xsl:template match="column[@type = 'datetime' and @default = 'sysdate'
                and /scheme[@dbtype = 'MYSQL']]"
            mode="typed-def-value">
        <xsl:text>CURRENT_TIMESTAMP</xsl:text>
    </xsl:template>

    <xsl:template match="column[@type = 'datetime' and @default = 'sysdate'
                and /scheme[@dbtype = 'ORACLE']]"
            mode="typed-def-value">
        <xsl:text>SYSDATE</xsl:text>
    </xsl:template>

    <xsl:template match="column" mode="autoinc" />

    <xsl:template match="column[/scheme/@dbtype = 'MYSQL'
            and primarykey and (../../@sequence or ../../@autoinc)]"
            mode="autoinc">
        <xsl:text> AUTO_INCREMENT</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype = 'MYSQL'
            and @type = 'decimal']" mode="sql-type">
        <xsl:text>DECIMAL(12, 6)</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype != 'MYSQL'
            and @type = 'decimal']" mode="sql-type">
        <xsl:text>NUMBER</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype = 'MYSQL'
            and @type = 'datetime']" mode="sql-type">
        <xsl:text>TIMESTAMP</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype != 'MYSQL'
            and @type = 'datetime']" mode="sql-type">
        <xsl:text>DATE</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype = 'MYSQL'
            and @type = 'longint']" mode="sql-type">
        <xsl:text>INT</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype != 'MYSQL'
            and @type = 'longint']" mode="sql-type">
        <xsl:text>NUMBER</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype = 'ORACLE'
            and @type = 'string']" mode="sql-type">
        <xsl:text>VARCHAR2(</xsl:text>
        <xsl:value-of select="size" />
        <xsl:text>)</xsl:text>
    </xsl:template>

    <xsl:template match="column[/scheme/@dbtype != 'ORACLE'
            and @type = 'string']" mode="sql-type">
        <xsl:text>VARCHAR(</xsl:text>
        <xsl:value-of select="size" />
        <xsl:text>)</xsl:text>
    </xsl:template>

</xsl:stylesheet>
