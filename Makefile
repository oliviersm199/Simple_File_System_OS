CFLAGS = -c -g -Wall -std=gnu99 `pkg-config fuse --cflags --libs`

LDFLAGS = `pkg-config fuse --cflags --libs`

# Uncomment on of the following three lines to compile
#SOURCES= disk_emu.c sfs_api.c sfs_test.c sfs_api.h
#SOURCES= disk_emu.c sfs_api.c sfs_test2.c sfs_api.h
SOURCES= disk_emu.c sfs_api.c fuse_wrappers.c sfs_api.h

#SOURCES = disk_emu.c sfs_api.c jit_test.c sfs_api.h # TEST 1, Create disk + simple read and write. 


OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=Olivier_SimardMorissette_sfs

all: $(SOURCES) $(HEADERS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	gcc $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	gcc $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *~ $(EXECUTABLE)
