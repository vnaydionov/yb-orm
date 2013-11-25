# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

DIRS = lib src

if DIALECT_SET
EXAMPLES_DIRS = examples
else
EXAMPLES_DIRS =
endif

if DIALECT_SET
if TUTORIAL_PRESENT
TUTORIAL_DIRS = tutorial
else
TUTORIAL_DIRS =
endif
else
TUTORIAL_DIRS =
endif


SUBDIRS = $(DIRS) $(EXAMPLES_DIRS) $(TUTORIAL_DIRS)

