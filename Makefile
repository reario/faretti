.SUFFIXES : .o.c

CC = gcc
# librerie e include per modbus
INCDIR = /home/reario/include
LIBDIR = /home/reario/lib

# librerie e include per postgres
LIBDB = /usr/local/pgsql/lib
DBINCDIR = /usr/include/postgresql/
#objs = value.o operate.o setplctime.o
#op = operate.o bit.o

all: faretti newf t

faretti: bit.o faretti.o
	$(CC) -Wall -L${LIBDIR} -lmodbus $^ -o $@

newf: newf.o
	$(CC) -Wall -L${LIBDIR} -lmodbus $^ -o $@

t: t.o
	$(CC) -Wall -L${LIBDIR} -lmodbus $^ -o $@

# vengono costruiti file object
.c.o: gh.h
	$(CC) -c -g -DOTB -DDOINSERT3 -Wall -I${DBINCDIR} -I$(INCDIR)/modbus $< -o $@

# cancella i file non necessari e pulisce la directory, pronta per una compilazione pulita
clean :
	rm -f *~ *.o *.i *.s *.core faretti newf t




########################################################################################################################
# myserver: myserver.o
#       $(CC) -Wall -I${INCDIR}/modbus -I${DBINCDIR} -L${LIBDIR} -lmodbus -lpq $^ -o $@
#scatti: db.o scatti.o
#       $(CC) -Wall -I${INCDIR}/modbus -I${DBINCDIR} -L${LIBDIR} -L${DBLIBDIR} -lmodbus -lncurses -lmysqlclient $^ -o $@
#secondi: db.o secondi.o
#       $(CC) -Wall -I${INCDIR}/modbus -I${DBINCDIR} -L${LIBDIR} -L${DBLIBDIR} -lmodbus -lncurses -lmysqlclient $^ -o $@
#enum: enum.o
#       $(CC) -g -Wall -L${LIBDIR} -lgsl -lgslcblas -lm $^ -o $@
#SCRIPT_SH = value.sh scatti.sh secondi.sh ss.sh
########################################################################################################################
