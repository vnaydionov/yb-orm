
#message(STATUS
#    "DEBUG: SDIR=${SDIR}, BDIR=${BDIR}")

if (UNIX)
    set(ENV{PATH} "${BDIR}/src/util:${BDIR}/src/orm:${BDIR}/src/yborm_gen:$ENV{PATH}")
else ()
    set(ENV{PATH} "${BDIR}/src/util;${BDIR}/src/orm;${BDIR}/src/yborm_gen;$ENV{PATH}")
endif ()

EXECUTE_PROCESS(COMMAND yborm_gen --domain
    ${SDIR}/examples/ex2_schema.xml ${BDIR}/examples/domain)
