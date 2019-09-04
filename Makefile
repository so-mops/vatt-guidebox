#################################################
# makefile for indilib gb-indi   		#
################directories######################

GB_INDI_OBJS = gb_serial.o gb_indi.o
GB_STANDALONE_OBJS = gb_serial.o gb_standalone.o

###############binaries####################

all: gb_indi gb_standalone

gb_indi: $(GB_INDI_OBJS)
	gcc $^ -lindidriver -lm -o indi-vatt-guidebox

gb_standalone: $(GB_STANDALONE_OBJS)
	gcc $^ -o vatt-guidebox

gb_serial.o: gb_serial.c
	gcc -c gb_serial.c

gb_indi.o: gb_indi.c
	gcc -c gb_indi.c

gb_standalone.o: gb_standalone.c
	gcc -c gb_standalone.c




###############utils#######################
clean: \
;rm *.o indi-vatt-guidebox vatt-guidebox



