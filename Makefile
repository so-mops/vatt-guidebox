#################################################
# makefile for indilib gb-indi   		#
################objects######################

GB_INDI_OBJS = gb_serial.o gb_indi.o gb_commands.o
GB_STANDALONE_OBJS = gb_serial.o gb_standalone.o gb_commands.o
GB_NG_OBJS =  ng_server.o gb_serial.o gb_commands.o
GB_NG_OBJS = test.o ng_server.o gb_serial.o gb_commands.o
GCCFLAGS=-std=gnu99

###############binaries####################

all: gb_standalone gb_indi gb_ng

gb_indi: $(GB_INDI_OBJS)
	gcc $(GCCFLAGS) $^ -lindidriver -lm -o indi-vatt-guidebox

gb_standalone: $(GB_STANDALONE_OBJS)
	gcc $(GCCFLAGS) $^ -o vatt-guidebox

gb_ng: $(GB_NG_OBJS)
	gcc $(GCCFLAGS) $^ -o gb_ng

gb_serial.o: gb_serial.c
	gcc $(GCCFLAGS) -c gb_serial.c

gb_commands.o: gb_commands.c
	gcc $(GCCFLAGS) -c gb_commands.c

gb_indi.o: gb_indi.c
	gcc $(GCCFLAGS) -c gb_indi.c

gb_standalone.o: gb_standalone.c 
	gcc $(GCCFLAGS) -c gb_standalone.c

ng_server.o: ng_server.c
	gcc $(GCCFLAGS) -c ng_server.c

test.o:	test.c
	gcc $(GCCFLAGS) -c test.c

# Not using make install, moving away from /usr/local/bin
install:
	cp ./indi-vatt-guidebox /usr/local/bin/

###############utils#######################
clean: \
;rm *.o indi-vatt-guidebox vatt-guidebox gb_ng



