#################################################
# makefile for indilib gb-indi   		#
################objects######################

GB_INDI_OBJS = gb_serial.o gb_indi.o gb_commands.o
GB_STANDALONE_OBJS = gb_serial.o gb_standalone.o gb_commands.o
GB_NG_OBJS = test.o ng_server.o

###############binaries####################

all: gb_standalone gb_indi

gb_indi: $(GB_INDI_OBJS)
	gcc $^ -lindidriver -lm -o indi-vatt-guidebox

gb_standalone: $(GB_STANDALONE_OBJS)
	gcc $^ -o vatt-guidebox

gb_ng: $(GB_NG_OBJS)
	gcc $^ -o gb_ng

gb_serial.o: gb_serial.c
	gcc -c gb_serial.c

gb_commands.o: gb_commands.c
	gcc -c gb_commands.c

gb_indi.o: gb_indi.c
	gcc -c gb_indi.c

gb_standalone.o: gb_standalone.c 
	gcc -c gb_standalone.c

lantronix.o: lantronics.c
	gcc -c lantronics.c

ng_server.o: ng_server.c
	gcc -c ng_server.c

test.o:	test.c
	gcc -c test.c




###############utils#######################
clean: \
;rm *.o indi-vatt-guidebox vatt-guidebox



