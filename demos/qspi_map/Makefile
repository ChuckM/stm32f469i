#
# This file is part of the libopencm3 project.
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.
#

OBJS = ../util/led.o ../util/console.o ../util/clock.o \
	   ../util/qspi.o ../util/retarget.o ../util/hexdump.o 

BINARY = main

LDSCRIPT = ../stm32f469i-discovery.ld
CLEANFILES += ${OBJS} ${OBJS:%.o=%.d}

CFLAGS = -I../../simple-graphics/include
LDFLAGS = -L../../simple-graphics/lib
LDLIBS = -lgfx

include ../../Makefile.include
